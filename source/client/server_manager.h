#pragma once
#include "session_manager.h"

class server_manager : public session_manager
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "server_manager";
    }

private:

};