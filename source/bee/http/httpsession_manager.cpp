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
#include "runnable.h"
#ifdef _REENTRANT
#include "threadpool.h"
#endif
#include "types.h"
#include "uri.h"

namespace bee
{

class http_task : public runnable
{
public:
    http_task(HTTP_TASKID taskid, httprequest* req, httpresponse* rsp)
        : _taskid(taskid), _req(req), _rsp(rsp)
    {
        if(_req) req->set_taskid(_taskid);
        if(_rsp) rsp->set_taskid(_taskid);
    }

    FORCE_INLINE HTTP_TASKID get_taskid() const { return _taskid; }

    virtual void destroy() override
    {
        delete _req;
        delete _rsp;
        runnable::destroy();
    }

protected:
    HTTP_TASKID _taskid  = 0;
    httprequest*  _req  = nullptr;
    httpresponse* _rsp  = nullptr;
};

class http_callback_task : public http_task
{
public:
    http_callback_task(HTTP_TASKID taskid, httprequest* req, httpresponse* rsp, int status)
        : http_task(taskid, req, rsp), _status(status) {}

    virtual void run() override
    {
        if(_req->_callback)
        {
            _req->_callback(_status, _req, _rsp);
        }
    }

protected:
    int _status = HTTP_STATUS_OK;
};

// 处理请求的task
class http_servlet_task : public http_task
{
public:
    http_servlet_task(HTTP_TASKID taskid, httprequest* req, httpresponse* rsp, TIMETYPE timeout)
        : http_task(taskid, req, rsp), _timeout(timeout) {}

    virtual void run() override
    {
        if(!_req || !_rsp) return;

        auto* httpserver = static_cast<httpserver_manager*>(_req->_manager);
        if(!httpserver) return;
        if(auto* dispatcher = httpserver->get_dispatcher())
        {
            dispatcher->handle(_req, _rsp);
        }
    }

protected:
    TIMETYPE _timeout = 0;
};

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
    return iter != _sessions.end() ? static_cast<httpsession*>(iter->second) : nullptr;
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
    _request_timeout = cfg->get<size_t>(identity(), "request_timeout", 30);
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
    if(auto busy_iter = _busy_connections.find(sid); busy_iter != _busy_connections.end())
    {
        local_log("httpclient_manager %s on_del_session, sid %lu found in busy connections, requestid %ld, remove it.", identity(), sid, busy_iter->second);
        _busy_connections.erase(busy_iter);
    }
    advance_all();
}

void httpclient_manager::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httpresponse::TYPE) return;

    if(auto* rsp = static_cast<httpresponse*>(protocol))
    {
        handle_response(rsp);
    }
}

int httpclient_manager::send_request(HTTP_METHOD method, const std::string& url, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    if(url.empty())
    {
        local_log("httpclient_manager %s send_request failed, url is empty.", identity());
        return HTTP_RESULT_INVALID_URL;
    }
    return send_request(method, uri(url), std::move(cbk), timeout, headers, body);
}

int httpclient_manager::send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    if(!uri.is_valid())
    {
        return HTTP_RESULT_INVALID_URL;
    }

    bool is_ssl = (uri.get_scheme() == "https");
    if(is_ssl && !this->ssl_enabled())
    {
        return HTTP_RESULT_SSL_NOT_ENABLED;
    }

    httprequest* req = httpprotocol::get_request();
    req->set_version(HTTP_VERSION_1_1);
    req->set_method(method);
    req->set_path(uri.get_path());
    req->set_query(uri.get_query());
    req->set_fragment(uri.get_fragment());
    req->set_body(body);
    req->set_requestid(++_next_requestid);
    req->set_callback(std::move(cbk));

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

    return send_request(req, cbk, timeout);
}

int httpclient_manager::send_request(httprequest* req, httprequest::callback cbk, TIMETYPE timeout/*ms*/)
{
    int requestid = req->get_requestid();
    TIMETYPE now = systemtime::get_time();

    bee::rwlock::wrscoped l(_locker);
    if(requestid == 0)
    {
        requestid = ++_next_requestid;
        req->set_requestid(requestid);
    }

    _requests_cache.emplace(requestid, req);
    _waiting_requests.emplace_back(requestid);
    _request_timeouts.emplace(now + (timeout > 0 ? timeout : _request_timeout), requestid);

    advance_all();
    
    return HTTP_RESULT_OK;
}

int httpclient_manager::get(const std::string& url, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_GET, url, std::move(cbk), timeout, std::move(headers));
}

int httpclient_manager::post(const std::string& url, const std::string& body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_POST, url, std::move(cbk), timeout, std::move(headers), body);
}

int httpclient_manager::put(const std::string& url, const std::string& body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_PUT, url, std::move(cbk), timeout, std::move(headers), body);
}

int httpclient_manager::del(const std::string& url, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_DELETE, url, std::move(cbk), timeout, std::move(headers));
}

int httpclient_manager::head(const std::string& url, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_HEAD, url, std::move(cbk), timeout, std::move(headers));
}

int httpclient_manager::options(const std::string& url, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_OPTIONS, url, std::move(cbk), timeout, std::move(headers));
}

int httpclient_manager::patch(const std::string& url, std::string body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_PATCH, url, std::move(cbk), timeout, std::move(headers), std::move(body));
}

int httpclient_manager::trace(const std::string& url, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_TRACE, url, std::move(cbk), timeout, std::move(headers));
}

void httpclient_manager::add_http_task(int status, httprequest* req, httpresponse* rsp)
{
    HTTP_TASKID taskid = ++_next_http_taskid;
    auto* task = new http_callback_task(taskid, req, rsp, status);

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

void httpclient_manager::handle_response(httpresponse* rsp)
{
    const SID sid = rsp->_sid;
    bool is_keepalive = rsp->is_keepalive();

    bee::rwlock::wrscoped l(_locker);
    
    auto busy_iter = _busy_connections.find(sid);
    if(busy_iter == _busy_connections.end())
    {
        local_log("httpclient_manager %s handle_response failed, sid %lu not found in busy connections", identity(), sid);
        return;
    }

    REQUESTID requestid = busy_iter->second;
    auto req_iter = _requests_cache.find(requestid);
    if(req_iter == _requests_cache.end())
    {
        _busy_connections.erase(busy_iter);
        recycle_connection(sid, is_keepalive);
        local_log("httpclient_manager %s handle_response failed, request not found for sid %lu, maybe request is timeout.", identity(), sid);
        return;
    }

    httprequest* req = req_iter->second;
    if(!req)
    {
        _busy_connections.erase(busy_iter);
        _requests_cache.erase(req_iter);
        recycle_connection(sid, rsp->is_keepalive());
        local_log("httpclient_manager %s handle_response failed, request is null for sid %lu", identity(), sid);
        return;
    }

    add_http_task(rsp->get_status(), req, rsp);

    // 清理状态
    _busy_connections.erase(busy_iter);
    _requests_cache.erase(req_iter);
    recycle_connection(sid, req->is_keepalive() && is_keepalive);

    // 如果有等待的请求和空闲连接，继续处理
    advance_all();
}

void httpclient_manager::recycle_connection(SID sid, bool is_keepalive)
{
    if(is_keepalive)
    {
        // 如果响应是长连接，则将连接标记为空闲
        _idle_connections.insert(sid);
        local_log("%s %lu session, httpprotocol handle finished, keep-alive, add to idle connections.", identity(), sid);
    }
    else
    {
        // 如果响应不是长连接，则关闭会话
        if(auto* ses = find_session_nolock(sid))
        {
            ses->close();
            local_log("%s %lu session, httpprotocol handle finished, not keep-alive, close connection.", identity(), sid);
        }
    }
}

void httpclient_manager::try_new_connection()
{
    if(!check_connection_count()) return;
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

void httpclient_manager::check_request_timeouts()
{
    TIMETYPE now = systemtime::get_time();

    for(auto iter = _request_timeouts.begin(); iter != _request_timeouts.end(); )
    {
        if(iter->second <= now)
        {
            // 超时处理
            REQUESTID requestid = iter->first;
            auto req_iter = _requests_cache.find(requestid);
            if(req_iter != _requests_cache.end())
            {
                if(httprequest* req = req_iter->second)
                {
                    add_http_task(HTTP_STATUS_REQUEST_TIMEOUT, req, nullptr);
                    _requests_cache.erase(req_iter);
                }
            }
            iter = _request_timeouts.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void httpclient_manager::advance_all()
{
    while(!_waiting_requests.empty() && !_idle_connections.empty())
    {
        // 获取等待的请求和空闲连接
        REQUESTID requestid = _waiting_requests.front();
        _waiting_requests.pop_front();
        
        const SID sid = *_idle_connections.begin();
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

            // 发送请求
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
    config* cfg = config::get_instance();
    _http_task_timeout = cfg->get<TIMETYPE>(identity(), "http_task_timeout", 30); // 默认30秒

    _dispatcher = new servlet_dispatcher;
    _dispatcher->add_servlet("/_/config", new config_servlet);
}

void httpserver_manager::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httprequest::TYPE) return;

    if(auto* req = dynamic_cast<httprequest*>(protocol))
    {
        handle_request(req);
    }
}

void httpserver_manager::send_response(SID sid, httpresponse* rsp)
{
    bee::rwlock::wrscoped l(_locker);
    if(auto* ses = find_session_nolock(sid))
    {
        send_response_nolock(ses, *rsp);
    }
}

void httpserver_manager::start_http_task(httprequest* req, httpresponse* rsp)
{
    HTTP_TASKID taskid = ++_next_http_taskid;
    auto* task = new http_servlet_task(taskid, req, rsp, systemtime::get_time() + _http_task_timeout);

    bee::rwlock::wrscoped l(_locker);
    _http_tasks.emplace(taskid, task);
}

void httpserver_manager::finish_http_task(HTTP_TASKID taskid)
{
    http_task* task = nullptr;
    {
        bee::rwlock::wrscoped l(_locker);

        auto iter = _http_tasks.find(taskid);
        if(iter == _http_tasks.end())
        {
            local_log("httpclient_manager %s finish_http_task failed, taskid %lu not found.", identity(), taskid);
            return;
        }
        task = iter->second;
        _http_tasks.erase(iter);
    }

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

void httpserver_manager::start_http_task_nolock(http_task* task)
{
    if(!task) return;
    task->set_timeout(systemtime::get_time() + _http_task_timeout);
    _http_tasks.emplace(task->get_taskid(), task);
}

void httpserver_manager::finish_http_task_nolock(HTTP_TASKID taskid)
{
    auto iter = _http_tasks.find(taskid);
    if(iter == _http_tasks.end())
    {
        local_log("httpclient_manager %s finish_http_task failed, taskid %lu not found.", identity(), taskid);
        return;
    }
    http_task* task = iter->second;
    _http_tasks.erase(iter);

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

void httpserver_manager::check_http_task_timeouts()
{
    TIMETYPE now = systemtime::get_time();
    bee::rwlock::wrscoped l(_locker);

    for(auto iter = _http_tasks.begin(); iter != _http_tasks.end();)
    {
        if(iter->second->get_taskid() == 0) // 未设置taskid的任务
        {
            ++iter;
            continue;
        }

        if(now >= iter->second->get_timeout())
        {
            local_log("httpsession_manager %s check_http_task_timeouts, taskid %lu timeout.", identity(), iter->second->get_taskid());
            iter->second->run();
            iter = _http_tasks.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void httpserver_manager::handle_request(httprequest* req)
{
    httpresponse* rsp = httpprotocol::get_response();

    // 初始化响应基本信息
    rsp->set_version(req->get_version());
    rsp->set_header("server", identity());
    
    auto* task = create_http_task(req, rsp);
    start_http_task_nolock(task);
}

} // namespace bee