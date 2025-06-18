#pragma once
#include "session_manager.h"

namespace bee
{

class client_manager : public session_manager, public singleton_support<client_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "client";
    }

private:

};

} // namespace bee