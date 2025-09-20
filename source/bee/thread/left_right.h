#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "types.h"

// see cassobee/doc/CppCon/How to make your data structures wait-free for reads - Pedro Ramalhete - CppCon 2015.pdf

namespace bee
{

enum LEFT_RIGHT
{
    READS_LEFT  = 0,
    READS_RIGHT = 1,
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
    struct ALIGN_CACHELINE_SIZE align_counter_t { std::atomic<size_t> value{0}; };

    ri_distributed_counter()
    {
        constexpr static int DEFAULT_COUNTER_NUMS = 8;
        _counters_nums = std::thread::hardware_concurrency();
        if(_counters_nums == 0)
        {
            _counters_nums = DEFAULT_COUNTER_NUMS;
        }
        _counters = new align_counter_t[_counters_nums * CACHELINE_WORDS];
    }

    ~ri_distributed_counter()
    {
        delete[] _counters;
    }

    virtual void arrive() override
    {
        _counters[thread_to_idx()].value.fetch_add(1);
    }

    virtual void depart() override
    {
        _counters[thread_to_idx()].value.fetch_sub(1);
    }

    virtual bool is_empty() override
    {
        int idx = _acquire_load.load();
        for(; idx < _counters_nums * (int)CACHELINE_WORDS; idx += CACHELINE_WORDS)
        {
            if(_counters[idx].value.load(std::memory_order_relaxed) > 0)
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
    align_counter_t*  _counters = nullptr;
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
        _left_inst->~T();
        _right_inst->~T();
        delete _left_inst;
        delete _right_inst;
    }

    template<typename Fn, typename ...Args>
    requires std::invocable<Fn, T*, Args...>
    auto apply_read(Fn&& func, Args&&... args)
    {
        const int vi = arrive();
        T* inst = _left_right.load() == READS_LEFT ? _left_inst : _right_inst;
        auto ret = std::invoke(std::forward<Fn>(func), inst, std::forward<Args>(args)...);
        depart(vi);
        return ret;
    }

    template<typename Fn, typename ...Args>
    requires std::invocable<Fn, T*, Args...>
    auto apply_write(Fn&& func, Args&&... args)
    {
        std::lock_guard<lock_type> lock(_writer_lock);
        if(_left_right.load(std::memory_order_relaxed) == READS_LEFT)
        {
            std::invoke(std::forward<Fn>(func), _right_inst, std::forward<Args>(args)...);
            _left_right.store(READS_RIGHT);
            toggle_version_and_wait();
            return std::invoke(std::forward<Fn>(func), _left_inst, std::forward<Args>(args)...);
        }
        else // WRITE
        {
            std::invoke(std::forward<Fn>(func), _left_inst, std::forward<Args>(args)...);
            _left_right.store(READS_LEFT);
            toggle_version_and_wait();
            return std::invoke(std::forward<Fn>(func), _right_inst, std::forward<Args>(args)...);
        }
    }

private:
    int arrive()
    {
        const int vi = _version_index.load();
        _read_indic[vi].arrive();
        return vi;
    }

    void depart(int vi)
    {
        _read_indic[vi].depart();
    }

    void toggle_version_and_wait()
    {
        const int local_vi  = _version_index.load();
        const int pre_vi = (int)(local_vi & 0x1);
        const int next_vi = (int)((local_vi + 1) & 0x1);
        // wait for readers from next version
        while(!_read_indic[next_vi].is_empty())
        {
            std::this_thread::yield();
        }
        // toggle the version index variable
        _version_index.store(next_vi);
        // wait for readers from previous version
        while(!_read_indic[pre_vi].is_empty())
        {
            std::this_thread::yield();
        }
    }

private:
    lrc_t _read_indic[2];
    std::atomic<int> _version_index = 0;
    std::atomic<int> _left_right = READS_LEFT;
    lock_type _writer_lock;
    T* _left_inst  = nullptr;
    T* _right_inst = nullptr;
};

} // namespace bee