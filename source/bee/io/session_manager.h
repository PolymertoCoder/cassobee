#pragma once
#include <stddef.h>
#include <bitset>
#include <unordered_map>

#include "common.h"
#include "lock.h"
#include "types.h"
#include "prot_define.h"

class session;
class address;
class protocol;
class octets;

class session_manager
{
public:
    session_manager();
    ~session_manager();
    void init();
    FORCE_INLINE virtual const char* identity() const { return "session_manager"; }
    FORCE_INLINE address* get_addr() { return _addr; }
    FORCE_INLINE int socktype() { return _socktype; }
    FORCE_INLINE int family() { return _family; }

    FORCE_INLINE bool check_protocol(PROTOCOLID type) { return _state._permited_protocols.test(type); }

    session* create_session();
    void add_session(SID sid, session* ses);
    void del_session(SID sid);
    session* find_session(SID sid);
    session* find_session_nolock(SID sid);

    void send_protocol(SID sid, const protocol& prot);
    void send_octets(SID sid, const octets& oct);

private:
    friend class session;
    struct state
    {
        std::bitset<MAXPROTOCOLID> _permited_protocols;
    } _state;
    
    int _socktype = 0;
    int _family = 0;
    address* _addr = nullptr;
    size_t _read_buffer_size = 0;
    size_t _write_buffer_size = 0;

    cassobee::rwlock _locker;
    std::unordered_map<SID, session*> _sessions;
};


void client(session_manager* manager);
void server(session_manager* manager);