#pragma once
#include <stddef.h>
#include <bitset>
#include <unordered_map>
#include <openssl/types.h>

#include "lock.h"
#include "types.h"
#include "prot_define.h"

namespace bee
{

class session;
class address;
class protocol;
class octets;

enum SESSION_TYPE
{
    SESSION_TYPE_NONE = -1,
    SESSION_TYPE_TCP,
    SESSION_TYPE_UDP,
    SESSION_TYPE_HTTP,
    SESSION_TYPE_HTTPS,
};

class session_manager
{
public:
    session_manager() = default;
    virtual ~session_manager();
    virtual void init();
    FORCE_INLINE virtual const char* identity() const { return "session_manager"; }
    FORCE_INLINE address* get_addr() { return _addr; }
    FORCE_INLINE int socktype() { return _socktype; }
    FORCE_INLINE int family() { return _family; }
    void set_addr(address* addr);

    FORCE_INLINE bool check_connection_count() { return _config.max_connections ? _sessions.size() < _config.max_connections : true; }
    FORCE_INLINE bool check_protocol(PROTOCOLID type) { return !_config.forbidden_protocols.test(type); }
    void check_timeouts();

    virtual void connect(); // as client
    virtual void listen();  // as server
    virtual void on_add_session(SID sid);
    virtual void on_del_session(SID sid);
    virtual void reconnect();

    virtual session* create_session();
    SID get_next_sessionid();
    void add_session(SID sid, session* ses);
    void del_session(SID sid);
    virtual session* find_session(SID sid);
    void close_session(SID sid);

    void add_session_nolock(SID sid, session* ses);
    void del_session_nolock(SID sid);
    session* find_session_nolock(SID sid);

    void send_protocol(SID sid, const protocol& prot);
    void send_octets(SID sid, const octets& oct);

    // ssl
    bool init_ssl(bool is_server);
    FORCE_INLINE bool ssl_enabled() const { return _ssl_ctx != nullptr; }
    FORCE_INLINE SSL_CTX* get_ssl_ctx() const { return _ssl_ctx; }

protected:
    friend class session;
    struct
    {
        size_t max_connections = 0;
        std::bitset<MAXPROTOCOLID + 1> forbidden_protocols;
    } _config;

    char _session_type = SESSION_TYPE_NONE;

    int _socktype = 0;
    int _family = 0;
    address* _addr = nullptr;
    size_t _read_buffer_size = 0;
    size_t _write_buffer_size = 0;
    short  _keepalive_timeout = 0; // 会话保活超时时间

    bee::rwlock _locker;
    SID _next_sessionid = 0;
    std::unordered_map<SID, session*> _sessions;

    // ssl
    bool _ssl_server = false;
    SSL_CTX* _ssl_ctx = nullptr;
    std::string _cert_path;
    std::string _key_path;
};

} // namespace bee