#pragma once
#include "types.h"
#include <pthread.h>
#include <unistd.h>
#include <immintrin.h>

#ifdef _REENTRANT
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <thread>
#endif

namespace bee
{

template<typename lock_type>
class lock_support
{
public:
    struct scoped
    {
        explicit scoped(lock_type& locker) noexcept : _locker(locker)
        {
            _locker.lock();
        }
        ~scoped() noexcept
        {
            _locker.unlock();
        }
        lock_type& _locker;
    };
};

template<typename rwlock_type>
class rwlock_support
{
public:
    struct rdscoped
    {
        explicit rdscoped(rwlock_type& locker) noexcept : _locker(locker)
        {
            _locker.rdlock();
        }
        ~rdscoped() noexcept
        {
            _locker.unlock();
        }
        rwlock_type& _locker;
    };
    struct wrscoped
    {
        explicit wrscoped(rwlock_type& locker) noexcept : _locker(locker)
        {
            _locker.wrlock();
        }
        ~wrscoped() noexcept
        {
            _locker.unlock();
        }
        rwlock_type& _locker;
    };
};

class empty_lock : public lock_support<empty_lock>
{
public:
    FORCE_INLINE void lock() {}
    FORCE_INLINE void unlock() {}
    FORCE_INLINE bool try_lock() { return true; }
    struct rdscoped { rdscoped(empty_lock&) {} };
    struct wrscoped { wrscoped(empty_lock&) {} };
    FORCE_INLINE void rdlock() {}
    FORCE_INLINE void try_rdlock() {}
    FORCE_INLINE void wrlock() {}
    FORCE_INLINE void try_wrlock() {}
};

#ifndef _REENTRANT
typedef empty_lock spinlock;
typedef empty_lock atomic_spinlock;
typedef empty_lock mutex;
typedef empty_lock rwlock;
#else

// 比较适合高竞争情景。低竞争场景下，可能会有额外的开销（如 pause 和 yield）
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

class atomic_spinlock : public lock_support<atomic_spinlock>
{
public:
    void lock()
    {
        while(_flag.test_and_set(std::memory_order_acquire)); 
    }
    bool try_lock()
    {
        return !_flag.test_and_set(std::memory_order_acquire);
    }
    void unlock()
    {
         _flag.clear(std::memory_order_release); 
    }

private:
    std::atomic_flag _flag = false;
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

class rwlock : public rwlock_support<rwlock>
{
public:
    rwlock()
    {
        if(pthread_rwlock_init(&_locker, NULL) != 0)
        {
            throw std::runtime_error("failed to initialize rwlock");
        }
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

#endif // #ifndef _REENTRANT
} // namespace bee