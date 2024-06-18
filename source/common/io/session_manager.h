#pragma once
#include <map>
#include "lock.h"
#include "types.h"
#include "address.h"

class session;

class session_manager
{
public:
    session_manager();

    session* find_session(SID sid);
    session* find_session_nolock(SID sid);

    void add_session();
    void remove_session();

public:
    int _socktype = 0;
    address* _local = nullptr;
    size_t _read_buffer_size = 0;
    size_t _write_buffer_size = 0;

    cassobee::rwlock _locker;
    std::map<SID, session*> _sessions;
};