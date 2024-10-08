#pragma once
#include <vector>
#include <list>
#include <functional>
#include "lock.h"
#include "macros.h"

namespace cassobee
{

// concurrent changelist: 基于双缓冲的并发changelist实现
// lock-free read，适用于单读多写的场景，锁粒度较小
template<typename value_type, template<typename, typename...> class container_type = std::list>
class cc_changelist
{
public:
    struct value_node
    {
        value_type value;
        Operation  op;
    };

    using list_type = container_type<value_node>;
    using read_callback = std::function<void(list_type&)>;

    auto read(const read_callback& func)
    {
        swap();
        auto& read_buf = get_read_buffer();
        func(read_buf);
        printf("cc_changelist read _writeidx=%d size=%zu\n", _writeidx, read_buf.size());
        read_buf.clear();
    }

    void write(const value_type& value, Operation op)
    {
        cassobee::spinlock::scoped l(_locker);
        auto& write_buf = get_write_buffer();
        write_buf.insert(std::cend(write_buf), value_node{value, op});
        printf("cc_changelist write _writeidx=%d size=%zu\n", _writeidx, write_buf.size());
    }

    FORCE_INLINE auto& get_write_buffer() { return _buffer[_writeidx];  }
    FORCE_INLINE auto& get_read_buffer()  { return _buffer[!_writeidx]; }

private:
    FORCE_INLINE void swap()
    {
        cassobee::spinlock::scoped l(_locker);
        _writeidx = 1 - _writeidx;
    }

private:
    cassobee::spinlock _locker;
    uint8_t   _writeidx = 0;
    list_type _buffer[2];
};

} // namespace cassobee;