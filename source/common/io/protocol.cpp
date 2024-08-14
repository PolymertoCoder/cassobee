#include "protocol.h"
#include "log.h"
#include "marshal.h"
#include "session.h"
#include "session_manager.h"
#include "threadpool.h"

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
        assert(false);
        return false;
    }
    if(manager->check_protocol(type))
    {
        local_log("protocol %d is forbidden by %s.", type, manager->identity());
        assert(false);
        return false;
    }
    return true;
}

void protocol::encode(octetsstream& os)
{

}

protocol* protocol::decode(octetsstream& os, session* ses)
{
    PROTOCOLID id = 0; size_t size = 0;
    try
    {
        os >> octetsstream::BEGIN >> id >> size;
        while(os.data_ready(size))
        {
            DEBUGLOG("protocol decode, data not enough, continue wait... id=%d size=%zu.", id, size);
            os >> octetsstream::ROLLBACK;
            return nullptr;
        }

        if(!check_policy(id, size, ses->get_manager()))
        {
            ERRORLOG("protocol check_policy failed, id=%d size=%zu.", id, size);
            assert(false);
        }

        if(protocol* temp = get_protocol(id))
        {
            temp->unpack(os);
            return temp;
        }
    }
    catch(...)
    {
        ERRORLOG("protocol decode failed, id=%d size=%zu.", id, size);
    }
    return nullptr;
}