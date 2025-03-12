#pragma once
#include "common.h"
#include "session_manager.h"

class logserver_manager : public session_manager, public singleton_support<logserver_manager>
{
public:
    FORCE_INLINE virtual const char* identity() const override
    {
        return "logserver";
    }
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    bool send(const protocol& prot);

    FORCE_INLINE bool is_connect() { return _localsid > 0; }

private:
    SID _localsid = 0;
};