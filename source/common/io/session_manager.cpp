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
    int version = cfg->get<int>(identity(), "version");
    if(version == 4) {
        _socktype = AF_INET;
    } else if(version == 6) {
        _socktype = AF_INET6;
    } else {
        assert(false);
    }

    _read_buffer_size  = cfg->get<size_t>(identity(), "read_buffer_size");
    _write_buffer_size = cfg->get<size_t>(identity(), "write_buffer_size");
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

        ses->wbuffer().append(os.data(), os.size());
        ses->permit_send();
    }
    else
    {
        ERRORLOG("session_manager %s cant find session %d on sending protocol", identity(), sid);
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
        ERRORLOG("session_manager %s cant find session %d on sending protocol", identity(), sid);
    }
}

void client(session_manager* manager)
{
    
}

void server(session_manager* manager)
{
    address* local = manager->local();
    switch(manager->family())
    {
        case AF_INET:
        {
            ipv4_address* localaddr = dynamic_cast<ipv4_address*>(local);
            int listenfd = socket(AF_INET, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in sock;
            bzero(&sock, sizeof(sock));
            sock.sin_family = AF_INET;
            sock.sin_addr.s_addr = htonl(INADDR_ANY);
            sock.sin_port = htons(localaddr->_addr.sin_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
        case AF_INET6:
        {
            ipv6_address* localaddr = dynamic_cast<ipv6_address*>(local);
            int listenfd = socket(AF_INET6, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in6 sock;
            bzero(&sock, sizeof(sock));
            sock.sin6_family = AF_INET;
            sock.sin6_addr = in6addr_any;
            sock.sin6_port = htons(localaddr->_addr.sin6_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
    }
}