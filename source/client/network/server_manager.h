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

    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;

    void send(const protocol& prot);

private:
    SID _localsid = 0;
};

} // namespace bee