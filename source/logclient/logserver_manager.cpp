#include "logserver_manager.h"
#include "address.h"
#include "glog.h"

namespace bee
{

void logserver_manager::on_add_session(SID sid)
{
    _localsid = sid;
    local_log("logserver_manager on_add_session %lu %s.", sid, get_addr()->to_string().data());
}

void logserver_manager::on_del_session(SID sid)
{
    _localsid = 0;
    local_log("logserver_manager on_del_session %lu.", sid);
    reconnect();
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

} // namespace bee