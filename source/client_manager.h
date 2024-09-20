#pragma once
#include "common.h"
#include "session_manager.h"

class client_manager : public session_manager, public singleton_support<client_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "client_manager";
    }

private:

};