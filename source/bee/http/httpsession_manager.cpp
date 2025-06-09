#include <algorithm>
#include <cctype>
#include <openssl/ssl.h>
#include "httpsession_manager.h"
#include "address.h"
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
#include "uri.h"

namespace bee
{

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

        // _pending_requests.emplace_back(req.dup(), cbk, systemtime::get_time() + _request_timeout);
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
    _idle_connections.insert(sid);
}

void httpclient_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);
    _idle_connections.erase(sid);
    if(auto iter = _busy_connections.find(sid); iter != _busy_connections.end())
    {
        REQUESTID requestid = iter->second;
        if(auto req_iter = _requests_cache.find(requestid); req_iter != _requests_cache.end())
        {
            req_iter->second->handle_response(HTTP_RESULT::WAIT_CLOSE_BY_PEER, nullptr);
            _requests_cache.erase(req_iter);
        }
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

auto httpclient_manager::get_new_requestid() -> httprequest::REQUESTID
{
    static httprequest::REQUESTID requestid = 0;
    return requestid++;
}

void httpclient_manager::try_new_connection(const uri& uri)
{
    std::string host = _dns.host.empty() ? uri.get_host() : _dns.host;
    int32_t port = (_dns.port == 0) ? uri.get_port() : _dns.port;

    address* addr = address::lookup_any(host, AF_INET, SOCK_STREAM);
    if(!addr)
    {
        local_log("session_manager %s cant find address %s", identity(), host.data());
        return;
    }
    addr->set_port(port);
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
    while(_waiting_requests.size())
    {
        REQUESTID requestid = _waiting_requests.front();
        _waiting_requests.pop_front();
        
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

int httpserver_manager::send_response()
{

}

} // namespace bee