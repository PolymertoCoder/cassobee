#pragma once
#include <map>
#include "lock.h"
#include "types.h"
#include "address.h"

class session;

class session_manager
{
public:
    session_manager()
    {
        
    }

    session* find_session(SID sid);
    session& find_session_nolock(SID sid);

protected:
    int _socktype;
    address* _local = nullptr;

    cassobee::rwlock _locker;
    std::map<SID, session*> _sessions;
};