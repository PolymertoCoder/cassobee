#include "logclient_manager.h"

void logclient_manager::on_add_session(SID sid)
{
    _localsid = sid;
}

void logclient_manager::on_del_session(SID sid)
{
    _localsid = 0;
}

bool logclient_manager::send(const protocol& prot)
{
    if(_localsid > 0)
    {
        send_protocol(_localsid, prot);
        return true;
    }
    return false;   
}