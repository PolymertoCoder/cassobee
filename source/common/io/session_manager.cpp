#include "session_manager.h"

#include "address.h"
#include "common.h"
#include "ioevent.h"
#include "log.h"
#include "macros.h"
#include "marshal.h"
#include "reactor.h"
#include "config.h"
#include "protocol.h"
#include "session.h"

session_manager::session_manager()
{
    auto cfg = config::get_instance();
    std::string socktype = cfg->get(identity(), "socktype");
    if(socktype == "tcp")
    {
        _socktype = SOCK_STREAM;
        int version = cfg->get<int>(identity(), "version");
        int port = cfg->get<int>(identity(), "port");
        if(version == 4)
        {
            _family = AF_INET;
            struct sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            addr.sin_family = _family;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
            _addr = address_factory::get_instance()->create("ipv4_address", addr);
        }
        else if(version == 6)
        {
            _family = AF_INET6;
            struct sockaddr_in6 addr;
            bzero(&addr, sizeof(addr));
            addr.sin6_family = AF_INET;
            addr.sin6_port = htons(port);
            addr.sin6_addr = in6addr_any;
            _addr = address_factory::get_instance()->create("ipv6_address", addr);
        }
        else
        {
            assert(false);
        }
    }
    else if(socktype == "udp")
    {
        _socktype = SOCK_DGRAM;
        // TODO
    }

    _read_buffer_size  = cfg->get<size_t>(identity(), "read_buffer_size");
    _write_buffer_size = cfg->get<size_t>(identity(), "write_buffer_size");
}

session* session_manager::create_session()
{
    return new session(this);
}

void session_manager::add_session(SID sid, session* ses)
{
    cassobee::rwlock::wrscoped l(_locker);
    ASSERT(!_sessions.contains(sid) && ses);
    _sessions.emplace(sid, ses);
}

void session_manager::del_session(SID sid)
{
    cassobee::rwlock::wrscoped l(_locker);
    _sessions.erase(sid);
}

session* session_manager::find_session(SID sid)
{
    cassobee::rwlock::rdscoped l(_locker);
    return find_session_nolock(sid);
}

session* session_manager::find_session_nolock(SID sid)
{
    auto iter = _sessions.find(sid);
    return iter != _sessions.end() ? iter->second : nullptr;
}

void session_manager::send_protocol(SID sid, const protocol& prot)
{
    cassobee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        thread_local octetsstream os;
        os.clear();
        prot.encode(os);

        octets& wbuffer = ses->wbuffer();
        if(os.size() > wbuffer.free_space())
        {
            ERRORLOG("session_manager %s, session %lu sid wbuffer is full on sending protocol", identity(), sid);
            return;
        }

        wbuffer.append(os.data(), os.size());
        ses->permit_send();
    }
    else
    {
        ERRORLOG("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }
}

void session_manager::send_octets(SID sid, const octets& oct)
{
    cassobee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        ses->wbuffer().append(oct.data(), oct.size());
        ses->permit_send();
    }
    else
    {
        ERRORLOG("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }
}

void client(session_manager* manager)
{
    
}

void server(session_manager* manager)
{
    reactor::get_instance()->add_event(new passiveio_event(manager), EVENT_ACCEPT);
    switch(manager->family())
    {
        case AF_INET:
        {
            
        } break;
        case AF_INET6:
        {
            
            reactor::get_instance()->add_event(new passiveio_event(manager), EVENT_ACCEPT);
        } break;
    }
}