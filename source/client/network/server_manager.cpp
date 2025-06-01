#include "server_manager.h"
#include "session.h"

namespace bee
{

void server_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);
    _localsid = sid;
}

void server_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);
    if(sid == _localsid)
    {
        _localsid = 0;
    }
}

void server_manager::send(const protocol& prot)
{
    if(_localsid)
    {
        send_protocol(_localsid, prot);
    }
}

} // namespace bee