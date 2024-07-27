#pragma once
#include <stddef.h>
#include <unordered_map>

#include "lock.h"
#include "types.h"

class session;
class address;
class protocol;
class octets;

class session_manager
{
public:
    session_manager();
    FORCE_INLINE virtual const char* identity() { return "session_manager"; }
    FORCE_INLINE address* local() { return _local; }
    FORCE_INLINE int family() { return _socktype; }

    void add_session(SID sid, session* ses);
    void del_session(SID sid);
    session* find_session(SID sid);
    session* find_session_nolock(SID sid);

    void send_protocol(SID sid, const protocol& prot);
    void send_octets(SID sid, const octets& oct);

private:
    friend class session;
    int _socktype = 0;
    address* _local = nullptr;
    size_t _read_buffer_size = 0;
    size_t _write_buffer_size = 0;

    cassobee::rwlock _locker;
    std::unordered_map<SID, session*> _sessions;
};


void client(session_manager* manager);
void server(session_manager* manager);