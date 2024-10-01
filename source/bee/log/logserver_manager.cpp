#include "logserver_manager.h"
#include "log.h"

void logserver_manager::on_add_session(SID sid)
{
    _localsid = sid;
    printf("logserver_manager on_add_session %lu.\n", sid);
}

void logserver_manager::on_del_session(SID sid)
{
    _localsid = 0;
    printf("logserver_manager on_del_session %lu.\n", sid);
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