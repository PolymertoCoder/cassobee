#include "session_manager.h"

#include "address.h"
#include "ioevent.h"
#include "log.h"
#include "marshal.h"
#include "reactor.h"
#include "config.h"
#include "protocol.h"
#include "session.h"

session_manager::session_manager()
{
}

session_manager::~session_manager()
{
    delete _addr;
    for(auto [sid, ses] : _sessions)
    {
        delete ses;
    }
    _sessions.clear();
}

void session_manager::on_add_session(SID sid)
{
}

void session_manager::on_del_session(SID sid)
{
}

session* session_manager::create_session()
{
    return new session(this);
}

void session_manager::init()
{
    auto cfg = config::get_instance();
    std::string socktype = cfg->get(identity(), "socktype");
    if(socktype == "tcp")
    {
        _socktype = SOCK_STREAM;
        int version = cfg->get<int>(identity(), "version");
        int port = cfg->get<int>(identity(), "port");
        TRACELOG("version:%d port:%d.", version, port);
        if(version == 4)
        {
            _family = AF_INET;
            _addr = address_factory::create2<"ipv4_address">("0.0.0.0", port);
        }
        else if(version == 6)
        {
            _family = AF_INET6;
            _addr = address_factory::create2<"ipv6_address">("0.0.0.0", port);
        }
        else
        {
            assert(false);
        }
        assert(_addr != nullptr);
    }
    else if(socktype == "udp")
    {
        _socktype = SOCK_DGRAM;
        // TODO
    }

    _read_buffer_size  = cfg->get<size_t>(identity(), "read_buffer_size");
    _write_buffer_size = cfg->get<size_t>(identity(), "write_buffer_size");
}

void session_manager::add_session(SID sid, session* ses)
{
    cassobee::rwlock::wrscoped l(_locker);
    ASSERT(!_sessions.contains(sid) && ses);
    _sessions.emplace(sid, ses);
    on_add_session(sid);
    TRACELOG("session_manager add_session %lu.", sid);
}

void session_manager::del_session(SID sid)
{
    cassobee::rwlock::wrscoped l(_locker);
    _sessions.erase(sid);
    on_del_session(sid);
    TRACELOG("session_manager del_session %lu.", sid);
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
    reactor::get_instance()->add_event(new activeio_event(manager), EVENT_SEND);
}

void server(session_manager* manager)
{
    reactor::get_instance()->add_event(new passiveio_event(manager), EVENT_ACCEPT);
}