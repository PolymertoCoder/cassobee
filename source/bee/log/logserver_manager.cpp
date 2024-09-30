#include "logserver_manager.h"
#include "log.h"

void logserver_manager::on_add_session(SID sid)
{
    _localsid = sid;
    local_log("logserver_manager on_add_session %lu.", sid);
}

void logserver_manager::on_del_session(SID sid)
{
    _localsid = 0;
    local_log("logserver_manager on_del_session %lu.", sid);
}

bool logserver_manager::send(const protocol& prot)
{
    if(_localsid > 0)
    {
        send_protocol(_localsid, prot);
        return true;
    }
    return false;   
}