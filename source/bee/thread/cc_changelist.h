#pragma once
#include <vector>
#include <list>
#include <functional>
#include "lock.h"
#include "macros.h"

#define USE_ATOMIC_FLAG_LOCK 1

namespace cassobee
{

// concurrent changelist: 基于双缓冲的并发changelist实现
// lock-free read，适用于单读多写的场景，锁粒度较小
template<typename value_type, template<typename, typename...> class container_type = std::list>
class ALIGN_CACHELINE_SIZE cc_changelist
{
public:
    struct value_node
    {
        value_type value;
        Operation  op;
        bool operator<(const value_node& rhs) const { return value < rhs.value; }
    };

    using list_type = container_type<value_node>;
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
        printf("cc_changelist read _readidx=%d read size=%zu\n", old_writeidx, read_buf.size());
        read_buf.clear();
    }

    void write(const value_type& value, Operation op)
    {
        lock_guard l(_locker);
        auto& write_buf = _buffer[_writeidx];
        write_buf.insert(std::cend(write_buf), value_node{value, op});
        std::atomic_thread_fence(std::memory_order_release);
        printf("cc_changelist write _writeidx=%d write size=%zu\n", _writeidx, write_buf.size());
    }

private:
    uint8_t _writeidx = 0;
#if USE_ATOMIC_FLAG_LOCK
    using lock_guard = cassobee::atomic_spinlock::scoped;
    cassobee::atomic_spinlock _locker;
#else
    using lock_guard = cassobee::spinlock::scoped;
    cassobee::spinlock _locker;
#endif
    list_type _buffer[2];
};

} // namespace cassobee;