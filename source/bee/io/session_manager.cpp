#include <openssl/ssl.h>
#include "session_manager.h"
#include "address.h"
#include "glog.h"
#include "ioevent.h"
#include "lock.h"
#include "marshal.h"
#include "reactor.h"
#include "config.h"
#include "protocol.h"
#include "session.h"

namespace bee
{

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
    local_log("session_manager %s, on_add_session %lu. ", identity(), sid);
}

void session_manager::on_del_session(SID sid)
{
    local_log("session_manager %s, on_del_session %lu. ", identity(), sid);
}

void session_manager::reconnect()
{
    add_timer(2000, [this]()
    {
        client(this);
        local_log("session_manager::reconnect run");
        return false;
    });
}

session* session_manager::create_session()
{
    session* ses = new session(this);
    ses->_sid = get_next_sessionid();
    return ses;
}

SID session_manager::get_next_sessionid()
{
    static sequential_id_generator<SID, bee::spinlock> sid_generator;
    return sid_generator.gen();
}

void session_manager::init()
{
    auto cfg = config::get_instance();
    std::string socktype = cfg->get(identity(), "socktype");
    if(socktype == "tcp")
    {
        _socktype = SOCK_STREAM;
        int version = cfg->get<int>(identity(), "version");
        auto address = cfg->get(identity(), "address");
        int port = cfg->get<int>(identity(), "port");
        local_log("version:%d address:%s port:%d.", version, address.data(), port);
        if(version == 4)
        {
            _family = AF_INET;
            _addr = address_factory::create2<"ipv4_address">(address.data(), port);
        }
        else if(version == 6)
        {
            _family = AF_INET6;
            _addr = address_factory::create2<"ipv6_address">(address.data(), port);
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
    _keepalive_timeout = cfg->get<TIMETYPE>(identity(), "keepalive_timeout", 30000);

    // ssl
    if(cfg->get<bool>(identity(), "ssl_enabled", false))
    {
        _ssl_server = cfg->get<bool>(identity(), "ssl_server", false);
        assert(init_ssl(_ssl_server));
    }
    else
    {
        local_log("SSL is disabled for %s", identity());
    }
}

void session_manager::add_session(SID sid, session* ses)
{
    bee::rwlock::wrscoped l(_locker);
    add_session_nolock(sid, ses);
}

void session_manager::del_session(SID sid)
{
    bee::rwlock::wrscoped l(_locker);
    del_session_nolock(sid);
}

session* session_manager::find_session(SID sid)
{
    bee::rwlock::rdscoped l(_locker);
    return find_session_nolock(sid);
}

void session_manager::add_session_nolock(SID sid, session* ses)
{
    ASSERT(!_sessions.contains(sid) && ses);
    _sessions.emplace(sid, ses);
    on_add_session(sid);
    local_log("session_manager add_session %lu.", sid);
}

void session_manager::del_session_nolock(SID sid)
{
    _sessions.erase(sid);
    on_del_session(sid);
    local_log("session_manager del_session %lu.", sid);
}

session* session_manager::find_session_nolock(SID sid)
{
    auto iter = _sessions.find(sid);
    return iter != _sessions.end() ? iter->second : nullptr;
}
    
void session_manager::send_protocol(SID sid, const protocol& prot)
{
    bee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        thread_local octetsstream os;
        os.clear();
        prot.encode(os);

        bee::rwlock::wrscoped sesl(ses->_locker);
        if(os.size() > ses->_writeos.data().free_space())
        {
            local_log("session_manager %s, session %lu write buffer is fulled.", identity(), sid);
            return;
        }

        ses->_writeos.data().append(os.data(), os.size());
        ses->permit_send();
    }
    else
    {
        local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }
}

void session_manager::send_octets(SID sid, const octets& oct)
{
    bee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        bee::rwlock::wrscoped sesl(ses->_locker);
        if(oct.size() > ses->_writeos.data().free_space())
        {
            local_log("session_manager %s, session %lu write buffer is fulled.", identity(), sid);
            return;
        }

        ses->_writeos.data().append(oct.data(), oct.size());
        ses->permit_send();
    }
    else
    {
        local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }
}

bool session_manager::init_ssl(bool is_server)
{
    auto cfg = config::get_instance();
    if(is_server)
    {
        _cert_path = cfg->get(identity(), "cert_file");
        _key_path  = cfg->get(identity(), "key_file");

        if(_cert_path.empty() || _key_path.empty())
        {
            return false;
        }

        _ssl_ctx = SSL_CTX_new(TLS_server_method());
        if(!_ssl_ctx)
        {
            return false;
        }

        SSL_CTX_set_options(_ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

        if(SSL_CTX_use_certificate_file(_ssl_ctx, _cert_path.data(), SSL_FILETYPE_PEM) <= 0)
        {
            local_log("session_manager %s load certificate failed.", identity());
            return false;
        }
        
        if(SSL_CTX_use_PrivateKey_file(_ssl_ctx, _key_path.data(), SSL_FILETYPE_PEM) <= 0)
        {
            local_log("session_manager %s load private key failed.", identity());
            return false;
        }

        local_log("server SSL context initialized for %s", identity());
    }
    else // client
    {
        _ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_load_verify_locations(_ssl_ctx, "ca.crt", nullptr);
        local_log("client SSL context initialized for %s", identity());
    }
    return true;
}

void client(session_manager* manager)
{
    reactor::get_instance()->add_event(new activeio_event(manager));
}

void server(session_manager* manager)
{
    reactor::get_instance()->add_event(new passiveio_event(manager));
}

} // namespace bee