#pragma once
#include "common.h"
#include "session_manager.h"

namespace bee
{

class server_manager : public session_manager, public singleton_support<server_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "server";
    }

private:

};

} // namespace bee