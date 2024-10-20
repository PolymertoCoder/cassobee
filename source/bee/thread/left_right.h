#pragma once
#include <atomic>
#include <mutex>
#include <thread>

#include "types.h"

// see cassobee/doc/CppCon/How to make your data structures wait-free for reads - Pedro Ramalhete - CppCon 2015.pdf

enum LEFT_RIGHT
{
    LEFT  = 0,
    RIGHT = 1,
};

class read_indicator
{
public:
    virtual void arrive() = 0;   // mark the arrival(of a reader)
    virtual void depart() = 0;   // mark the departure(of a reader)
    virtual bool is_empty() = 0; // return false if there is a thread that has arrived but not yet departed
};

class ri_atomic_counter : public read_indicator
{
public:
    virtual void arrive() override
    {
        _counter.fetch_add(1);
    }

    virtual void depart() override
    {
        _counter.fetch_sub(1);
    }

    virtual bool is_empty() override
    {
        return _counter.load() == 0;
    }

private:
    std::atomic<uint64_t> _counter{0};
};

class ri_distributed_counter : public read_indicator
{
public:
    ri_distributed_counter()
    {
        constexpr static int DEFAULT_COUNTER_NUMS = 8;
        _counters_nums = std::thread::hardware_concurrency();
        if(_counters_nums == 0)
        {
            _counters_nums = DEFAULT_COUNTER_NUMS;
        }
        _counters = new std::atomic<size_t>[_counters_nums * CACHELINE_WORDS];
    }

    ~ri_distributed_counter()
    {
        delete[] _counters;
    }

    virtual void arrive() override
    {
        _counters[thread_to_idx()].fetch_add(1);
    }

    virtual void depart() override
    {
        _counters[thread_to_idx()].fetch_sub(1);
    }

    virtual bool is_empty() override
    {
        int idx = _acquire_load.load();
        for(; idx < _counters_nums * (int)CACHELINE_WORDS; idx += CACHELINE_WORDS)
        {
            if(_counters[idx].load(std::memory_order_relaxed) > 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                return false;
            }
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        return true;
    }

private:
    int thread_to_idx()
    {
        size_t idx = _hash_func(std::this_thread::get_id());
        return (int)((idx % _counters_nums) * CACHELINE_WORDS);
    }

private:
    int _counters_nums;
    std::atomic<size_t>*  _counters = nullptr;
    std::atomic<uint64_t> _acquire_load{0};
    std::hash<std::thread::id> _hash_func;
};

using lrc_t = ri_distributed_counter;

template<typename T, typename lock_type = std::mutex>
class left_right
{
public:
    template<typename ...Args>
    left_right(Args&&... args)
    {
        void* inst  = ::operator new(sizeof(T) * 2);
        _left_inst  = new(inst) T(std::forward<Args>(args)...);
        _right_inst = new((T*)inst + 1) T(std::forward<Args>(args)...);
    }

    ~left_right()
    {
        delete _left_inst;
        delete _right_inst;
    }

    template<typename Fn, typename ...Args>
    requires std::invocable<Fn, T*, Args...>
    auto apply_read(Fn func, Args&&... args)
    {
        const int vi = arrive();
        T* inst = _left_right.load() == LEFT ? _left_inst : _right_inst;
        auto ret = func(inst, std::forward<Args>(args)...);
        depart(vi);
        return ret;
    }

    template<typename Fn, typename ...Args>
    requires std::invocable<Fn, T*, Args...>
    auto apply_write(Fn func, Args&&... args)
    {
        std::lock_guard<lock_type> lock(_writer_lock);
        if(_left_right.load(std::memory_order_relaxed) == LEFT)
        {
            func(_right_inst, std::forward<Args>(args)...);
            _left_right.store(LEFT);
            toggle_version_and_wait();
            return func(_left_inst, std::forward<Args>(args)...);
        }
        else
        {
            func(_left_inst, std::forward<Args>(args)...);
            _left_right.store(RIGHT);
            toggle_version_and_wait();
            return func(_right_inst, std::forward<Args>(args)...);
        }
    }

private:
    int arrive()
    {
        const int vi = _version_index.load();
        _lrc[vi].arrive();
        return vi;
    }

    void depart(int vi)
    {
        _lrc[vi].depart();
    }

    void toggle_version_and_wait()
    {
        const int vi  = _version_index.load();
        const int pvi = (int)(vi & 0x1);
        const int nvi = (int)((vi + 1) & 0x1);
        // wait for readers from next version
        while(!_lrc[pvi].is_empty())
        {
            std::this_thread::yield();
        }
        // toggle the version index variable
        _version_index.store(nvi);
        // wait for readers from previous version
        while(!_lrc[nvi].is_empty())
        {
            std::this_thread::yield();
        }
    }

private:
    lrc_t _lrc[2];
    std::atomic<int> _version_index = 0;
    std::atomic<int> _left_right = LEFT;
    lock_type _writer_lock;
    T* _left_inst  = nullptr;
    T* _right_inst = nullptr;
};

