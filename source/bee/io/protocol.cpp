#include "protocol.h"
#include "marshal.h"
#include "session.h"
#include "session_manager.h"
#include "types.h"
#include "glog.h"
#include <cstdio>

namespace bee
{

void protocol::init_session(session* ses)
{
    if(!ses) return;
    _sid = ses->get_sid();
    _manager = ses->get_manager();
}

ostringstream& protocol::dump(ostringstream& out) const
{
    out << "type: " << _type << "\n";
    out << "maxsize: " << maxsize() << "\n";
    out << "sid: " << _sid << "\n";
    out << "priority: " << _priority << "\n";
    return out;
}

bool protocol::size_policy(PROTOCOLID type, size_t size)
{
    auto iter = get_map().find(type);
    return iter != get_map().end() && size <= iter->second->maxsize();
}

bool protocol::check_policy(PROTOCOLID type, size_t size, session_manager* manager)
{
    if(!size_policy(type, size))
    {
        local_log("protocol %d check size policy failed, size:%zu.", type, size);
        ASSERT(false);
        return false;
    }
    if(!manager->check_protocol(type))
    {
        local_log("protocol %d is forbidden by %s.", type, manager->identity());
        ASSERT(false);
        return false;
    }
    return true;
}

static size_t get_protocol_guess_size(PROTOCOLID type)
{
    // TODO: guess size by protocol type
    return 2048;
}

void protocol::encode(octetsstream& os) const
{
    PROTOCOLID id = get_type();
    size_t size = 0;

    try
    {
        os << compact_int(id);

        size_t guess_size = get_protocol_guess_size(id);
        size_t guess_size_size = get_compact_int_size(guess_size);

        thread_local std::vector<char> buf;
        buf.clear();
        buf.reserve(guess_size_size);

        size_t size_begin_pos = os.size();
        os.data().append(buf.data(), guess_size_size);

        size_t prev_size = os.size();
        pack(os);
        size = os.size() - prev_size;

        auto& data = os.data();
        size_t actual_size_size = get_compact_int_size(size);

        if(actual_size_size != guess_size_size)
        {
            size_t diff = actual_size_size - guess_size_size;
            data.reserve(data.size() + diff);
            memmove(data.begin() + prev_size + diff,
                    data.begin() + prev_size,
                    size);
            data.fast_resize(diff);

            local_log_f("protocol encode, actual_size_size({}) != guess_size_size({}), id={} size={}.",
                        actual_size_size, guess_size_size, id, size);
        }

        // 编码 size 到 buf
        buf.clear(); // 清空再用
        bee::encode_compact_int(size, buf);

        // 回填 compact 编码后的 size
        data.replace(size_begin_pos, buf.data(), actual_size_size);
    }
    catch(...)
    {
        local_log_f("protocol encode failed, id={} size={}.", id, size);
    }
}

protocol* protocol::decode(octetsstream& os, session* ses)
{
    if(!os.data_ready(1)) return nullptr;

    compact_int<PROTOCOLID> id = 0;
    compact_int<size_t> size = 0;

    try
    {
        os >> octetsstream::BEGIN >> id >> size;

        if(!os.data_ready(size))
        {
            local_log_f("protocol decode, data not enough, continue wait... id={} size={} actual_size={} pos={}.",
                        id, size, os.size(), os.get_pos());
            os >> octetsstream::ROLLBACK;
            return nullptr;
        }
        // local_log("id:%d decode size:%zu", id, size);

        // if(!check_policy(id, size, ses->get_manager()))
        // {
        //     local_log("protocol check_policy failed, id=%d size=%zu.", id, size);
        //     ASSERT(false);
        // }

        if(protocol* temp = get_protocol(id))
        {
            temp->init_session(ses);
            temp->unpack(os);
            return temp;
        }
    }
    catch(octetsstream::exception& e)
    {
        ses->set_close(SESSION_CLOSE_REASON_EXCEPTION);
        local_log_f("protocol decode throw octetesstream exception {}, id={} size={}.", e.what(), id, size);
    }
    catch(...)
    {
        ses->set_close(SESSION_CLOSE_REASON_EXCEPTION);
        local_log_f("protocol decode failed, id={} size={}.", id, size);
    }
    os >> octetsstream::ROLLBACK;
    return nullptr;
}

ostringstream& operator<<(ostringstream& oss, const protocol& prot)
{
    prot.dump(oss);
    return oss;
}

} // namespace bee