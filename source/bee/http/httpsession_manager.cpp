#include <openssl/ssl.h>
#include "httpsession_manager.h"
#include "httpsession.h"
#include "config.h"
#include "session_manager.h"
#include "glog.h"
#include "httpprotocol.h"
#include "systemtime.h"

namespace bee
{

httpsession_manager::httpsession_manager()
    : session_manager()
{
    _session_type = SESSION_TYPE_HTTP;
}
    
httpsession_manager::~httpsession_manager()
{
    if(_ssl_ctx)
    {
        SSL_CTX_free(_ssl_ctx);
    }
}

void httpsession_manager::init()
{
    session_manager::init();
    auto cfg = config::get_instance();
    _max_requests = cfg->get<int>(identity(), "max_requests");
    _request_timeout = cfg->get<int>(identity(), "request_timeout");
}

void httpsession_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);
    // TODO 动态保活配置
}

void httpsession_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);
}

void httpsession_manager::reconnect()
{
    
}

httpsession* httpsession_manager::create_session()
{
    auto ses = new httpsession(this);
    ses->set_sid(session_manager::get_next_sessionid());
    return ses;
}

httpsession* httpsession_manager::find_session(SID sid)
{
    bee::rwlock::rdscoped l(_locker);
    auto iter = _sessions.find(sid);
    return iter != _sessions.end() ? dynamic_cast<httpsession*>(iter->second) : nullptr;
}

// 请求头安全检查
bool httpsession_manager::check_headers(const httpprotocol* req)
{
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024; // 1MB
    if(req->get_header("Content-Length").size() > MAX_BODY_SIZE) return false;

    // 检查Host头
    if(!req->has_header("Host")) return false;

    // 限制头数量
    constexpr size_t MAX_HEADERS = 64;
    if(req->get_header_count() > MAX_HEADERS) return false;

    // 防御请求走私攻击
    if(req->has_header("Transfer-Encoding") && 
        req->has_header("Content-Length")) return false;

    // 检查头字段合法性
    static const std::set<std::string> ALLOWED_HEADERS = {
        "Host", "User-Agent", "Accept", "Content-Length", 
        "Content-Type", "Connection"
    };

    for(const auto& [k, v] : req->get_headers())
    {
        // 检查字段名合法性
        if (k.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos)
            return false;
        
        // 白名单检查
        if (!ALLOWED_HEADERS.contains(k)) return false;
    }
    return true;
}

void httpsession_manager::send_request(SID sid, const httprequest& req, httprequest::callback cbk)
{
    bee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        thread_local octetsstream os;
        os.clear();
        req.encode(os);

        bee::rwlock::wrscoped sesl(ses->_locker);
        if(os.size() > ses->_writeos.data().free_space())
        {
            local_log("session_manager %s, session %lu write buffer is fulled.", identity(), sid);
            return;
        }

        ses->_writeos.data().append(os.data(), os.size());
        ses->permit_send();

        _pending_requests.emplace_back(req.dup(), cbk, systemtime::get_time() + _request_timeout);
    }
    else
    {
        local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }    
}

void httpsession_manager::send_response(SID sid, const httpresponse& rsp)
{
    bee::rwlock::rdscoped l(_locker);
    if(session* ses = find_session_nolock(sid))
    {
        thread_local octetsstream os;
        os.clear();
        rsp.encode(os);

        bee::rwlock::wrscoped sesl(ses->_locker);
        if(os.size() > ses->_writeos.data().free_space())
        {
            local_log("session_manager %s, session %lu write buffer is fulled.", identity(), sid);
            return;
        }

        ses->_writeos.data().append(os.data(), os.size());
        ses->permit_send();
    }
    else
    {
        local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
    }   
}

} // namespace bee