#pragma once
#include "types.h"
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <immintrin.h>

namespace cassobee
{

template<typename lock_type>
class lock_support
{
public:
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
        //printf("spinlock %p lock.\n", this);
    }
    bool try_lock()
    {
        return !_atomic.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        _atomic.store(false, std::memory_order_release);
        //printf("spinlock %p lock.\n", this);
    }

private:
    std::atomic_bool _atomic = false;
};

class mutex : public lock_support<mutex>
{
public:
    FORCE_INLINE void lock()
    {
        _locker.lock();
    }
    FORCE_INLINE bool try_lock()
    {
        return _locker.try_lock();
    }
    FORCE_INLINE void unlock()
    {
        _locker.unlock();
    }

private:
    std::mutex _locker;
};

class rwlock
{
public:
    struct rdscoped
    {
        rdscoped(rwlock& locker) : _locker(locker)
        {
            _locker.rdlock();
        }
        ~rdscoped()
        {
            _locker.unlock();
        }
        rwlock& _locker;
    };
    struct wrscoped
    {
        wrscoped(rwlock& locker) : _locker(locker)
        {
            _locker.wrlock();
        }
        ~wrscoped()
        {
            _locker.unlock();
        }
        rwlock& _locker;
    };

    rwlock()
    {
        pthread_rwlock_init(&_locker, NULL);
    }
    ~rwlock()
    {
        pthread_rwlock_destroy(&_locker);
    }
    FORCE_INLINE void rdlock()
    {
        pthread_rwlock_rdlock(&_locker);
    }
    FORCE_INLINE void try_rdlock()
    {
        pthread_rwlock_tryrdlock(&_locker);
    }
    FORCE_INLINE void wrlock()
    {
        pthread_rwlock_wrlock(&_locker);
    }
    FORCE_INLINE void try_wrlock()
    {
        pthread_rwlock_trywrlock(&_locker);
    }
    FORCE_INLINE void unlock()
    {
        pthread_rwlock_unlock(&_locker);
    }

private:
    pthread_rwlock_t _locker;
};

}