#include "httpsession_manager.h"
#include "config.h"
#include "glog.h"

namespace bee
{

httpsession* httpsession_manager::create_session()
{
    auto ses = new httpsession(this);
    ses->set_sid(session_manager::get_next_sessionid());
    return ses;
}

void httpsession_manager::init()
{
    session_manager::init();
    if(ssl_enabled())
    {
        _session_type = SESSION_TYPE_HTTPS;
    }
    else
    {
        _session_type = SESSION_TYPE_HTTP;
    }

    config* cfg = config::get_instance();
    _http_task_timeout = cfg->get<TIMETYPE>(identity(), "http_task_timeout", 30); // 默认30秒
}

httpsession* httpsession_manager::find_session(SID sid)
{
    bee::rwlock::rdscoped l(_locker);
    auto iter = _sessions.find(sid);
    return iter != _sessions.end() ? static_cast<httpsession*>(iter->second) : nullptr;
}

void httpsession_manager::send_request_nolock(session* ses, const httprequest& req)
{
    if(!ses) return;

    thread_local octetsstream os;
    os.clear();
    req.encode(os);

    if(os.size() > ses->_writeos.data().free_space())
    {
        local_log("httpsession_manager %s, session %lu write buffer is fulled.", identity(), ses->get_sid());
        return;
    }

    ses->_writeos.data().append(os.data(), os.size());
    ses->permit_send();
}

void httpsession_manager::send_response_nolock(session* ses, const httpresponse& rsp)
{
    if(!ses) return;

    thread_local octetsstream os;
    os.clear();
    rsp.encode(os);

    if(os.size() > ses->_writeos.data().free_space())
    {
        local_log("httpsession_manager %s, session %lu write buffer is fulled.", identity(), ses->get_sid());
        return;
    }

    ses->_writeos.data().append(os.data(), os.size());
    ses->permit_send();
}

} // namespace bee