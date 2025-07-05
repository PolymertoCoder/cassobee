#include "protocol.h"
#include "bytes_order.h"
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

void protocol::encode(octetsstream& os) const
{
    PROTOCOLID id = get_type(); size_t size = 0;
    try
    {
        os << id;
        size_t size_begin_pos = os.size();
        os << size;
        size_t prev_size = os.size();
        pack(os);
        size = hostToNetwork(os.size() - prev_size);
        //local_log("id:%d encode size:%zu", id, os.size() - prev_size);
        os.data().replace(size_begin_pos, (char*)(&size), sizeof(size));
    }
    catch(...)
    {
        local_log("protocol encode failed, id=%d size=%zu.", id, size);
    }
}

protocol* protocol::decode(octetsstream& os, session* ses)
{
    if(!os.data_ready(1)) return nullptr;
    PROTOCOLID id = 0; size_t size = 0;
    try
    {
        os >> octetsstream::BEGIN >> id >> size;
        if(!os.data_ready(size))
        {
            local_log("protocol decode, data not enough, continue wait... id=%d size=%zu actual_size=%zu.", id, size, os.size());
            os >> octetsstream::ROLLBACK;
            return nullptr;
        }
        local_log("id:%d decode size:%zu", id, size);

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
        local_log("protocol decode throw octetesstream exception %s, id=%d size=%zu.", e.what(), id, size);
    }
    catch(...)
    {
        ses->set_close(SESSION_CLOSE_REASON_EXCEPTION);
        local_log("protocol decode failed, id=%d size=%zu.", id, size);
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