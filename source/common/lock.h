#pragma once
#include <atomic>
#include <thread>
#include <unistd.h>
#include <immintrin.h>

namespace cassobee
{

template<typename lock_type>
class lock_support
{
    struct scoped
    {
        scoped(lock_type& locker) : _locker(locker)
        {
            _locker.lock();
        }
        ~scoped()
        {
            _locker.unlock();
        }
        lock_type& _locker;
    };
};

class spinlock : public lock_support<spinlock>
{
public:
    spinlock() = default;
    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;
    void lock()
    {
        while(_atomic.exchange(true, std::memory_order_acquire)) // 这一行总线争用严重，因此增加了内层循环
        {
            while(true) // 只有在锁被释放后才会跳出内层循环，load是一组只读指令，资源消耗极少
            {
                _mm_pause(); // pause指令，延迟时间大概是12纳秒
                if(!_atomic.load(std::memory_order_relaxed)) break;
                std::this_thread::yield(); // 在无其他线程等待执行的情况下，延迟时间113纳秒
                // 在有其他线程等待执行情况下，将切换线程
                if(!_atomic.load(std::memory_order_relaxed)) break;
            }
        }
    }
    bool try_lock()
    {
        return !_atomic.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        _atomic.store(false, std::memory_order_release);
    }

private:
    std::atomic_bool _atomic;
};

class mutex : public lock_support<spinlock>
{
public:


};


}