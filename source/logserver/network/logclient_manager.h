#pragma once
#include "session_manager.h"

namespace bee
{

class logclient_manager : public session_manager, public singleton_support<logclient_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "logclient";
    }
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
};

} // namespace bee