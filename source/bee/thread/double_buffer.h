#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include "lock.h"
#include "systemtime.h"

#define DOUBLE_BUFFER_USE_ATOMIC_FLAG_LOCK 1
#define DOUBLE_BUFFER_ENABLE_STAT 1

namespace bee
{

// concurrent changelist: 基于双缓冲的并发changelist实现
// lock-free read，适用于单读多写的场景，锁粒度较小
template<typename value_type, template<typename, typename...> class container_type = std::list>
class ALIGN_CACHELINE_SIZE one_reader_double_buffer
{
public:
    using list_type = container_type<value_type>;
    using read_callback = std::function<void(list_type&)>;

    void read(const read_callback& func)
    {
        uint8_t old_writeidx;
        {
            lock_guard l(_locker);
            old_writeidx = _writeidx;
            _writeidx = 1 - _writeidx;
            std::atomic_thread_fence(std::memory_order_release);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        auto& read_buf = _buffer[old_writeidx];
        func(read_buf);
        // printf("one_reader_double_buffer read _readidx=%d read size=%zu\n", old_writeidx, read_buf.size());
        read_buf.clear();
    }

    void write(const value_type& value)
    {
        lock_guard l(_locker);
        auto& write_buf = _buffer[_writeidx];
        write_buf.insert(std::cend(write_buf), value);
        std::atomic_thread_fence(std::memory_order_release);
        // printf("one_reader_double_buffer write _writeidx=%d write size=%zu\n", _writeidx, write_buf.size());
    }

private:
    uint8_t _writeidx = 0;
#if DOUBLE_BUFFER_USE_ATOMIC_FLAG_LOCK
    using lock_guard = bee::atomic_spinlock::scoped;
    bee::atomic_spinlock _locker;
#else
    using lock_guard = bee::spinlock::scoped;
    bee::spinlock _locker;
#endif
    list_type _buffer[2];
};

template<typename T, typename Allocator = std::allocator<T>>
class concurrent_double_buffer
{
    struct alignas(CACHELINE_SIZE) buffer_node
    {
        std::atomic_size_t ref_count{0};
        std::vector<T, Allocator> data;
        std::atomic<uint64_t> version{0};
    };

    using buffer_ptr = std::shared_ptr<buffer_node>;
    
    // 双缓冲和版本控制
    alignas(CACHELINE_SIZE) std::atomic<buffer_ptr> _front;
    alignas(CACHELINE_SIZE) std::atomic<buffer_ptr> _back;
    alignas(CACHELINE_SIZE) std::atomic<uint64_t> _global_version{0};
    
    // 写锁和版本队列
    bee::ticket_spinlock _write_lock;
    alignas(CACHELINE_SIZE) std::atomic<uint64_t> _published_version{0};

    // 扩容锁和try接口条件变量
    alignas(CACHELINE_SIZE) mutable bee::ticket_spinlock _capacity_lock;
    std::condition_variable_any _data_cond;

#ifdef DOUBLE_BUFFER_ENABLE_STAT
    // 统计信息
    alignas(CACHELINE_SIZE) std::atomic_size_t _total_writes{0};
    alignas(CACHELINE_SIZE) std::atomic_size_t _total_reads{0};
    alignas(CACHELINE_SIZE) std::atomic_size_t _buffer_switches{0};
    alignas(CACHELINE_SIZE) std::atomic_size_t _max_observed_size{0};
#endif

public:
    explicit concurrent_double_buffer(size_t reserve_size = 1024)
    {
        auto buf = std::make_shared<buffer_node>();
        buf->data.reserve(reserve_size);
        _front.store(buf, std::memory_order_relaxed);
        _back.store(std::make_shared<buffer_node>(), std::memory_order_relaxed);
    }

    // 写操作接口
    template<typename... Args>
    void emplace(Args&&... args)
    {
        std::lock_guard<ticket_spinlock> lock(_write_lock);
        
        buffer_ptr back = _back.load(std::memory_order_relaxed);
        if(back->ref_count.load(std::memory_order_acquire) > 0)
        {
            rotate_buffers();
            back = _back.load(std::memory_order_relaxed);
        }
        
        try
        {
            back->data.emplace_back(std::forward<Args>(args)...);
        }
        catch (...)
        {
            handle_write_exception();
            throw;
        }
        
        _global_version.fetch_add(1, std::memory_order_release);
    }

    // 批量读操作接口
    template<typename Func>
    void consume(Func&& func)
    {
        const uint64_t current_version = _global_version.load(std::memory_order_acquire);
        buffer_ptr front = _front.load(std::memory_order_acquire);
        
        front->ref_count.fetch_add(1, std::memory_order_acq_rel);
        
        try
        {
            if(front->version.load(std::memory_order_acquire) < current_version)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                func(front->data);
                front->version.store(current_version, std::memory_order_release);
            }
        }
        catch (...)
        {
            front->ref_count.fetch_sub(1, std::memory_order_acq_rel);
            throw;
        }
        
        front->ref_count.fetch_sub(1, std::memory_order_acq_rel);
    }

    // 带等待的消费接口
    template<typename Func, typename Rep, typename Period>
    bool try_consume_for(Func&& func, int timeout)
    {
        const TIMETYPE start = systemtime::get_time();
        uint64_t last_version = _published_version.load(std::memory_order_acquire);
        
        while(true)
        {
            if(consume_impl(func, last_version))
            {
                return true;
            }
            
            if(systemtime::get_time() - start > timeout)
            {
                return false;
            }
            
            std::unique_lock<bee::ticket_spinlock> lock(_write_lock);
            _data_cond.wait_for(lock, timeout, [&] {
                return _published_version.load(std::memory_order_acquire) > last_version;
            });
            last_version = _published_version.load(std::memory_order_acquire);
        }
    }

    // 迭代器支持（只读快照）
    class const_iterator
    {
        typename std::vector<T>::const_iterator _it;
        buffer_ptr _anchor;

    public:
        const_iterator(buffer_ptr anchor, bool end = false)
            : _anchor(std::move(anchor)) 
        {
            if(_anchor)
            {
                _it = end ? _anchor->data.end() : _anchor->data.begin();
            }
        }

        const T& operator*() const { return *_it; }
        const T* operator->() const { return &*_it; }
        
        const_iterator& operator++()
        {
            ++_it;
            return *this;
        }

        bool operator==(const const_iterator& other) const { return _it == other._it; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    // 容量管理接口
    size_t capacity() const noexcept
    {
        bee::ticket_spinlock::scoped l(_capacity_lock);
        return _back.load(std::memory_order_relaxed)->data.capacity();
    }

    void reserve(size_t new_capacity)
    {
        std::lock_guard<ticket_spinlock> l(_capacity_lock);
        buffer_ptr back = _back.load(std::memory_order_relaxed);
        if(back->data.capacity() < new_capacity)
        {
            buffer_ptr new_buffer = std::make_shared<buffer_node>();
            new_buffer->data.reserve(new_capacity);
            _back.store(new_buffer, std::memory_order_release);
        }
    }

    // 清空所有缓冲区（线程安全）
    void clear() noexcept
    {
        std::lock_guard<ticket_spinlock> lock1(_write_lock);
        std::lock_guard<ticket_spinlock> lock2(_capacity_lock);
        
        auto new_buffer = std::make_shared<buffer_node>();
        new_buffer->data.reserve(_back.load(std::memory_order_relaxed)->data.capacity());
        
        _back.store(new_buffer, std::memory_order_relaxed);
        _front.store(new_buffer, std::memory_order_release);
        
        _global_version.store(0, std::memory_order_relaxed);
        _published_version.store(0, std::memory_order_release);
    }

    // 迭代器访问接口
    const_iterator cbegin() const
    {
        buffer_ptr front = _front.load(std::memory_order_acquire);
        front->ref_count.fetch_add(1, std::memory_order_acq_rel);
        return const_iterator(front);
    }

    const_iterator cend() const
    {
        buffer_ptr front = _front.load(std::memory_order_acquire);
        front->ref_count.fetch_add(1, std::memory_order_acq_rel);
        return const_iterator(front, true);
    }

    // 统计信息接口
    struct statistics
    {
        size_t writes;
        size_t reads;
        size_t buffer_switches;
        size_t max_observed_size;
        size_t current_version;
    };

    statistics get_stats() const noexcept
    {
        return
        {
        #ifdef DOUBLE_BUFFER_ENABLE_STAT
            _total_writes.load(std::memory_order_relaxed),
            _total_reads.load(std::memory_order_relaxed),
            _buffer_switches.load(std::memory_order_relaxed),
            _max_observed_size.load(std::memory_order_relaxed),
            _global_version.load(std::memory_order_relaxed)
        #endif
        };
    }

private:
    void rotate_buffers() noexcept
     {
        buffer_ptr old_back = _back.load(std::memory_order_relaxed);
        buffer_ptr new_buffer = std::make_shared<buffer_node>();
        new_buffer->data.reserve(old_back->data.capacity());
        
        _back.store(new_buffer, std::memory_order_relaxed);
        _front.store(old_back, std::memory_order_release);
        
        _published_version.store(
            _global_version.load(std::memory_order_acquire),
            std::memory_order_release
        );
    }

    [[noreturn]] void handle_write_exception()
    {
        try{
            rotate_buffers();
        } catch (...) {
            std::terminate();
        }
    }

    void check_capacity(buffer_ptr& buf) // 动态扩容策略
    {
        const size_t current_size = buf->data.size();
    #ifdef DOUBLE_BUFFER_ENABLE_STAT
        _max_observed_size.store(
            std::max(_max_observed_size.load(std::memory_order_relaxed), current_size),
            std::memory_order_relaxed
        );
    #endif

        if(current_size >= buf->data.capacity())
        {
            std::lock_guard<ticket_spinlock> lock(_capacity_lock);
            const size_t new_capacity = buf->data.capacity() * 2;
            buffer_ptr new_buffer = std::make_shared<buffer_node>();
            new_buffer->data.reserve(new_capacity);
            _back.store(new_buffer, std::memory_order_release);
        #ifdef DOUBLE_BUFFER_ENABLE_STAT
            _buffer_switches.fetch_add(1, std::memory_order_relaxed);
        #endif
        }
    }

    template<typename Func>
    bool consume_impl(Func&& func, uint64_t& last_version)
    {
        buffer_ptr front = _front.load(std::memory_order_acquire);
        front->ref_count.fetch_add(1, std::memory_order_acq_rel);
        
        if(front->version.load(std::memory_order_acquire) > last_version)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            try
            {
                func(front->data);
            #ifdef DOUBLE_BUFFER_ENABLE_STAT
                _total_reads.fetch_add(1, std::memory_order_relaxed);
            #endif
            }
            catch (...)
            {
                front->ref_count.fetch_sub(1, std::memory_order_acq_rel);
                throw;
            }
            last_version = front->version.load(std::memory_order_relaxed);
            front->ref_count.fetch_sub(1, std::memory_order_acq_rel);
            return true;
        }
        front->ref_count.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
};

} // namespace bee;