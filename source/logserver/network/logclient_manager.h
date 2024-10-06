#pragma once
#include "common.h"
#include "session_manager.h"

class logclient_manager : public session_manager, public singleton_support<logclient_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "logclient";
    }
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    bool send(const protocol& prot);

private:
    SID _localsid = 0;
};