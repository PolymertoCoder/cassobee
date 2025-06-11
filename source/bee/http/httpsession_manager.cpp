#include <algorithm>
#include <cctype>
#include <openssl/ssl.h>
#include "httpsession_manager.h"
#include "address.h"
#include "http.h"
#include "httpsession.h"
#include "config.h"
#include "lock.h"
#include "log.h"
#include "session_manager.h"
#include "servlet.h"
#include "config_servlet.h"
#include "glog.h"
#include "httpprotocol.h"
#include "systemtime.h"
#include "threadpool.h"
#include "uri.h"

namespace bee
{

class http_callback : public runnable
{
public:
    http_callback(int result, httprequest* req, httpresponse* rsp)
        : _result(result), _req(req), _rsp(rsp) {}

    virtual void run() override
    {
        if(_req->_callback)
        {
            _req->_callback(_result, _req, _rsp);
        }

        // 如果请求和响应都不是长连接，则关闭会话
        if(!_req->is_keepalive() || !_rsp->is_keepalive())
        {
            if(auto* ses = _rsp->_manager->find_session(_rsp->_sid))
            {
                ses->close();
                local_log("%s %lu session, httpprotocol handle finished, not keep-alive, close connection.", _rsp->_manager->identity(), _rsp->_sid);
            }
        }
    }

    virtual void destroy() override
    {
        delete _req;
        delete _rsp;
        runnable::destroy();
    }

protected:
    int _result = HTTP_RESULT::OK;
    httprequest*  _req = nullptr;
    httpresponse* _rsp = nullptr;
};

httpsession_manager::httpsession_manager()
    : session_manager()
{
    _session_type = SESSION_TYPE_HTTP;
}

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
    if(req->get_header("content-length").size() > MAX_BODY_SIZE) return false;

    // 检查Host头
    if(!req->has_header("host")) return false;

    // 限制头数量
    constexpr size_t MAX_HEADERS = 64;
    if(req->get_header_count() > MAX_HEADERS) return false;

    // 防御请求走私攻击
    if(req->has_header("transfer-encoding") && 
        req->has_header("content-length")) return false;

    // 检查头字段合法性
    static const std::set<std::string> ALLOWED_HEADERS = {
        "host", "user-agent", "accept", "content-length", 
        "content-type", "connection"
    };

    for(const auto& [k, v] : req->get_headers())
    {
        // 检查字段名合法性
        std::string key;
        std::ranges::transform(k, key.begin(), [](unsigned char c){ return std::tolower(c); });
        if(key.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos)
            return false;
        
        // 白名单检查
        if(!ALLOWED_HEADERS.contains(key)) return false;
    }
    return true;
}

void httpsession_manager::send_request_nolock(session* ses, const httprequest& req)
{
    if(!ses) return;

    thread_local octetsstream os;
    os.clear();
    req.encode(os);

    bee::rwlock::wrscoped sesl(ses->_locker);
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

    bee::rwlock::wrscoped sesl(ses->_locker);
    if(os.size() > ses->_writeos.data().free_space())
    {
        local_log("httpsession_manager %s, session %lu write buffer is fulled.", identity(), ses->get_sid());
        return;
    }

    ses->_writeos.data().append(os.data(), os.size());
    ses->permit_send();
}

httpclient_manager::~httpclient_manager()
{
    for(const auto& [_, req] : _requests_cache)
    {
        delete req;
    }
}

void httpclient_manager::init()
{
    httpsession_manager::init();
    auto cfg = config::get_instance();
    _max_requests = cfg->get<size_t>(identity(), "max_requests");
    _request_timeout = cfg->get<size_t>(identity(), "request_timeout");
    _uri = uri(cfg->get(identity(), "uri"));
    assert(_uri.is_valid());
    if(!refresh_dns()) assert(false);
}

void httpclient_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);

    bee::rwlock::wrscoped l(_locker);
    _idle_connections.insert(sid);
    advance_all();
}

void httpclient_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);

    bee::rwlock::wrscoped l(_locker);
    _idle_connections.erase(sid);
    if(auto b_iter = _busy_connections.find(sid); b_iter != _busy_connections.end())
    {
        REQUESTID requestid = b_iter->second;
        if(auto req_iter = _requests_cache.find(requestid); req_iter != _requests_cache.end())
        {
            req_iter->second->handle_response(HTTP_RESULT::WAIT_CLOSE_BY_PEER, nullptr);
            _requests_cache.erase(req_iter);
        }
    }
}

void httpclient_manager::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httpresponse::TYPE) return;
    if(auto* rsp = dynamic_cast<httpresponse*>(protocol))
    {
        handle_response(HTTP_RESULT::OK, rsp);
    }
}

int httpclient_manager::send_request(HTTP_METHOD method, const std::string& url, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    uri uri(url);
    if(!uri.is_valid())
    {
        return HTTP_RESULT::INVALID_URL;
    }
    return send_request(method, uri, std::move(cbk), timeout, headers, body);
}

int httpclient_manager::send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    httprequest* req = httpprotocol::get_request();
    req->set_version(HTTP_VERSION_1_1);
    req->set_method(method);
    req->set_path(uri.get_path());
    req->set_query(uri.get_query());
    req->set_fragment(uri.get_fragment());
    req->set_body(body);
    req->set_requestid(get_new_requestid());
    req->set_timeout(systemtime::get_time() + timeout);

    bool has_host = false;
    for(auto& [key, value] : headers)
    {
        if(strcasecmp(key.data(), "connection") == 0)
        {
            if(strcasecmp(value.data(), "keep-alive") == 0) 
            {
                req->set_keepalive(true);
            }
            continue;
        }

        if(!has_host && strcasecmp(key.data(), "host") == 0)
        {
            has_host = !value.empty();
        }

        req->set_header(key, value);
    }
    if(!has_host)
    {
        req->set_header("host", uri.get_host());
    }

    req->set_body(body);

    return send_request(req, uri, cbk, timeout);
}

int httpclient_manager::send_request(httprequest* req, const uri& uri, httprequest::callback cbk, TIMETYPE timeout/*ms*/)
{
    bool is_ssl = (uri.get_scheme() == "https");
    if(is_ssl && !this->ssl_enabled())
    {
        return HTTP_RESULT::SSL_NOT_ENABLED;
    }

    int requestid = req->get_requestid();
    {
        bee::rwlock::wrscoped l(_locker);
        if(requestid == 0)
        {
            requestid = get_new_requestid();
            req->set_requestid(requestid);
        }

        _requests_cache.emplace(requestid, req);
        _waiting_requests.emplace_back(requestid);

        advance_all();
    }
    
    return HTTP_RESULT::SEND_ASYNC;
}

httprequest* httpclient_manager::find_httprequest(REQUESTID requestid)
{
    auto iter = _requests_cache.find(requestid);
    return iter != _requests_cache.end() ? iter->second : nullptr;
}

httprequest* httpclient_manager::find_httprequest_by_sid(SID sid)
{
    return nullptr;
}

void httpclient_manager::handle_response(int result, httpresponse* rsp)
{
    bee::rwlock::wrscoped l(_locker);
    if(!rsp || rsp->_manager != this || rsp->_sid <= 0) return;

    auto b_iter = _busy_connections.find(rsp->_sid);
    if(b_iter == _busy_connections.end())
    {
        local_log("httpclient_manager %s handle_response failed, sid %lu not found in busy connections", identity(), rsp->_sid);
        return;
    }

    auto r_iter = _requests_cache.find(b_iter->second);
    if(r_iter == _requests_cache.end())
    {
        local_log("httpclient_manager %s handle_response failed, request not found for sid %lu", identity(), rsp->_sid);
        return;
    }

    httprequest* req = r_iter->second;
    if(!req)
    {
        local_log("httpclient_manager %s handle_response failed, request not found for sid %lu", identity(), rsp->_sid);
        return;
    }

    auto* task = new http_callback(HTTP_RESULT::OK, req, rsp);
#ifdef _REENTRANT
    threadpool::get_instance()->add_task(rsp->thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

void httpclient_manager::check_timeouts()
{
    bee::rwlock::wrscoped l(_locker);
    TIMETYPE now = systemtime::get_time();
    
    // 检查等待队列中的请求超时
    for(auto it = _waiting_requests.begin(); it != _waiting_requests.end(); )
    {
        if(auto req_iter = _requests_cache.find(*it); req_iter != _requests_cache.end())
        {
            if(req_iter->second->get_timeout() < now)
            {
                if(req_iter->second->_callback)
                {
                    req_iter->second->_callback(HTTP_RESULT::TIMEOUT, req_iter->second, nullptr);
                }
                _requests_cache.erase(req_iter);
                it = _waiting_requests.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    // 检查已发送但未响应的请求超时
    for(auto& [sid, requestid] : _busy_connections)
    {
        if (auto req_iter = _requests_cache.find(requestid); req_iter != _requests_cache.end())
        {
            if (req_iter->second->get_timeout() < now)
            {
                if (req_iter->second->_callback)
                {
                    req_iter->second->_callback(HTTP_RESULT::TIMEOUT, req_iter->second, nullptr);
                }
                _requests_cache.erase(req_iter);
            }
        }
    }
}

auto httpclient_manager::get_new_requestid() -> httprequest::REQUESTID
{
    static httprequest::REQUESTID requestid = 0;
    return requestid++;
}

void httpclient_manager::try_new_connection()
{
    connect();
}

bool httpclient_manager::refresh_dns(const std::string& uri_str)
{
    if(!uri_str.empty())
    {
        _uri = uri(uri_str);
    }

    if(!_uri.is_valid())
    {
        local_log("httpclient_manager %s refresh_dns failed, uri_str is invalid.", identity());
        return false;
    }

    _dns.host = _uri.get_host();
    _dns.port = _uri.get_port();
    address* addr = address::lookup_any(_dns.host, AF_INET, SOCK_STREAM);
    if(!addr)
    {
        local_log("httpclient_manager %s refresh_dns failed, cant find address %s", identity(), _dns.host.data());
        return false;
    }
    delete _addr;
    _addr = addr;

    local_log("httpclient_manager %s refresh_dns success, %s --> %s:%d", identity(), uri_str.data(), _dns.host.data(), _dns.port);
    return true;
}

void httpclient_manager::advance_all()
{
    while(!_waiting_requests.empty() && !_idle_connections.empty())
    {
        // 获取等待的请求和空闲连接
        REQUESTID requestid = _waiting_requests.front();
        _waiting_requests.pop_front();
        
        SID sid = *_idle_connections.begin();
        _idle_connections.erase(sid);
        
        auto req_iter = _requests_cache.find(requestid);
        if(req_iter == _requests_cache.end()) continue;

        httprequest* req = req_iter->second;
        if(!req) continue;

        // 发送请求
        if(session* ses = find_session_nolock(sid))
        {
            // 设置请求的会话ID
            req->init_session(ses);
            send_request_nolock(ses, *req);

            // 将连接标记为忙碌
            _busy_connections[sid] = requestid;
        }
        else
        {
            local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
        }
    }

    // 如果还有等待的请求但没有空闲连接，尝试创建新连接
    if(!_waiting_requests.empty() && _idle_connections.empty())
    {
        try_new_connection();
    }
}

httpserver_manager::~httpserver_manager()
{
    delete _dispatcher;
}

void httpserver_manager::init()
{
    httpsession_manager::init();
    _dispatcher = new servlet_dispatcher;
    _dispatcher->add_servlet("/_/config", new config_servlet);
}

void httpserver_manager::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httprequest::TYPE) return;
    if(auto* req = dynamic_cast<httprequest*>(protocol))
    {
        
    }
}

int httpserver_manager::send_response()
{

}

} // namespace bee