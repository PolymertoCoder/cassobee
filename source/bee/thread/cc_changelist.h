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

    auto read(const read_callback& func)
    {
        uint8_t old_writeidx = _writeidx.exchange(1 - _writeidx.load());
        auto& read_buf = _buffer[old_writeidx];
        func(read_buf);
        //printf("cc_changelist read _writeidx=%d size=%zu\n", _writeidx.load(), read_buf.size());
        read_buf.clear();
    }

    void write(const value_type& value, Operation op)
    {
        auto& write_buf = _buffer[_writeidx.load()];
        {
            cassobee::spinlock::scoped l(_locker);
            write_buf.insert(std::cend(write_buf), value_node{value, op});
        }
        //printf("cc_changelist write _writeidx=%d size=%zu\n", _writeidx.load(), write_buf.size());
    }

private:
    cassobee::spinlock _locker;
    std::atomic<uint8_t> _writeidx = 0;
    list_type _buffer[2];
};

} // namespace cassobee;