#include "protocol.h"
#include "threadpool.h"

bool protocol::check_policy(PROTOCOLID id, size_t size)
{
    auto iter = get_map().find(id);
    return iter != get_map().end() && size <= iter->second->maxsize;
}

void protocol::encode(octetsstream& os)
{

}

void protocol::decode(octetsstream& os)
{
    PROTOCOLID id = 0; size_t size = 0;
    while(os.data_ready(1))
    {
        os >> id >> size;

        if(!os.data_ready(size))
        {
            DEBUGLOG("protocol decode, data not enough, continue wait... id=%d size=%zu.", id, size);
            return;
        }

        if(!check_policy(id, size))
        {
            ERRORLOG("protocol check_policy failed, id=%d size=%zu.", id, size);
            assert(false);
        }

        auto* prot = get_protocol(id);
        prot->unpack(os);

        threadpool::get_instance()->add_task(prot->thread_group_idx(), [prot]()
        {
            prot->run();
            delete prot;
        });
    }
}