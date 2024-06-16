#pragma once
#include <map>
#include "types.h"
#include "address.h"

class session;

class session_manager
{
public:
    session_manager()
    {
        
    }

private:
    address* _local = nullptr;
    std::map<SID, session*> _sessions;
};