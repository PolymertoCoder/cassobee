#pragma once
#include <vector>
#include <list>
#include <functional>
#include "lock.h"
#include "macros.h"

#define USE_ATOMIC_FLAG_LOCK 0

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
        // 尝试设置锁标志
    #if USE_ATOMIC_FLAG_LOCK
        while(_locker.test_and_set(std::memory_order_acquire));
    #else
        cassobee::spinlock::scoped l(_locker);
    #endif
        uint8_t old_writeidx = _writeidx.exchange(1 - _writeidx.load(std::memory_order_acquire), std::memory_order_acq_rel);
        auto& read_buf = _buffer[old_writeidx];
        func(read_buf);
        //printf("cc_changelist read _readidx=%d read size=%zu\n", old_writeidx, read_buf.size());
        read_buf.clear();
        // 清除锁标志
    #if USE_ATOMIC_FLAG_LOCK
        _locker.clear(std::memory_order_release);
    #endif
    }

    void write(const value_type& value, Operation op)
    {
    #if USE_ATOMIC_FLAG_LOCK
        // 尝试设置锁标志
        while(_locker.test_and_set(std::memory_order_acquire));
    #else
        cassobee::spinlock::scoped l(_locker);
    #endif
        auto& write_buf = _buffer[_writeidx.load(std::memory_order_acquire)];
        write_buf.insert(std::cend(write_buf), value_node{value, op});
        //printf("cc_changelist write _writeidx=%d write size=%zu\n", _writeidx.load(std::memory_order_acquire), write_buf.size());
    #if USE_ATOMIC_FLAG_LOCK
        // 清除锁标志
        _locker.clear(std::memory_order_release);
    #endif
    }

private:
    std::atomic<uint8_t> _writeidx = 0;
#if USE_ATOMIC_FLAG_LOCK
    std::atomic_flag _locker = ATOMIC_FLAG_INIT; // 原子标志用于锁定
#else
    cassobee::spinlock _locker;
#endif
    list_type _buffer[2];
};

} // namespace cassobee;