#pragma once
#include <stddef.h>
#include <bitset>
#include <unordered_map>

#include "common.h"
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
    SESSION_TYPE_NORMAL,
    SESSION_TYPE_HTTP,
};

class session_manager
{
public:
    session_manager();
    virtual ~session_manager();
    virtual void init();
    FORCE_INLINE virtual const char* identity() const { return "session_manager"; }
    FORCE_INLINE address* get_addr() { return _addr; }
    FORCE_INLINE int socktype() { return _socktype; }
    FORCE_INLINE int family() { return _family; }

    FORCE_INLINE bool check_protocol(PROTOCOLID type) { return _state._permited_protocols.test(type); }

    virtual void on_add_session(SID sid);
    virtual void on_del_session(SID sid);
    virtual void reconnect();

    virtual session* create_session();
    SID get_next_sessionid();
    void add_session(SID sid, session* ses);
    void del_session(SID sid);
    virtual session* find_session(SID sid);

    void add_session_nolock(SID sid, session* ses);
    void del_session_nolock(SID sid);
    session* find_session_nolock(SID sid);

    void send_protocol(SID sid, const protocol& prot);
    void send_octets(SID sid, const octets& oct);

    FORCE_INLINE bool is_httpsession_manager() const { return _session_type == SESSION_TYPE_HTTP; }

protected:
    friend class session;
    struct state
    {
        std::bitset<MAXPROTOCOLID> _permited_protocols;
    } _state;

    uint8_t _session_type = SESSION_TYPE_NORMAL;

    int _socktype = 0;
    int _family = 0;
    address* _addr = nullptr;
    size_t _read_buffer_size = 0;
    size_t _write_buffer_size = 0;

    size_t _keepalive_timeout = 0; // 会话保活超时时间

    bee::rwlock _locker;
    std::unordered_map<SID, session*> _sessions;
};


void client(session_manager* manager);
void server(session_manager* manager);

} // namespace bee