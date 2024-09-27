#pragma once
#include "common.h"
#include "session_manager.h"

class logserver_manager : public session_manager, public singleton_support<logserver_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "logserver_manager";
    }

private:

};