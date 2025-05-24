#include <algorithm>
#include <cctype>
#include <openssl/ssl.h>
#include "httpsession_manager.h"
#include "address.h"
#include "httpsession.h"
#include "config.h"
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

void httpsession_manager::check_timeouts()
{

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

}

void httpclient_manager::init()
{
    session_manager::init();
    auto cfg = config::get_instance();
    _max_requests = cfg->get<int>(identity(), "max_requests");
    _request_timeout = cfg->get<int>(identity(), "request_timeout");
}

void httpclient_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);
    _idle_connections.erase(sid);
    //_busy_connections.insert(sid);
}

void httpclient_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);
    _idle_connections.erase(sid);
    if(auto iter = _busy_connections.find(sid); iter != _busy_connections.end())
    {
        REQUESTID requestid = iter->second;
    }
}

http_result httpclient_manager::send_request(HTTP_METHOD method, const std::string& url, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    uri uri(url);
    if(!uri.is_valid())
    {
        return http_result(http_result::ERR::INVALID_URL, nullptr, "invalid url");
    }
    return send_request(method, uri, std::move(cbk), timeout, headers, body);
}

http_result httpclient_manager::send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
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

http_result httpclient_manager::send_request(httprequest* req, const uri& uri, httprequest::callback cbk, TIMETYPE timeout/*ms*/)
{
    bool is_ssl = (uri.get_scheme() == "https");
    if(is_ssl && !this->ssl_enabled())
    {
        return http_result(http_result::ERR::SSL_NOT_ENABLED, nullptr, "ssl not enabled");
    }
    if(req->get_requestid() == 0)
    {
        req->set_requestid(get_new_requestid());
    }

    //_pending_requests.emplace_back(req, timeout, cbk);
}

httprequest* httpclient_manager::find_httprequest(REQUESTID requestid)
{
    auto iter = _requests_cache.find(requestid);
    return iter != _requests_cache.end() ? iter->second : nullptr;
}

httprequest* httpclient_manager::find_httprequest_by_sid(SID sid)
{
    
}

auto httpclient_manager::get_new_requestid() -> httprequest::REQUESTID
{
    static httprequest::REQUESTID requestid = 0;
    return requestid++;
}

void httpclient_manager::try_new_connection(httprequest* req, const uri& uri, TIMETYPE timeout/*ms*/)
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

void httpclient_manager::refresh_dns()
{

}

httpserver_manager::~httpserver_manager()
{
    delete _dispatcher;
}

void httpserver_manager::init()
{
    session_manager::init();
    _dispatcher = new servlet_dispatcher;
    _dispatcher->add_servlet("/_/config", new config_servlet);
}

void httpserver_manager::send_response()
{

}

} // namespace bee