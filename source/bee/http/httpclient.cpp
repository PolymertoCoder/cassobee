#include "httpclient.h"
#include "config.h"
#include "systemtime.h"
#include "http_callback.h"
#include "address.h"
#ifdef _REENTRANT
#include "threadpool.h"
#endif

namespace bee
{

void httpclient::init()
{
    httpsession_manager::init();
    auto cfg = config::get_instance();
    _uri = uri(cfg->get(identity(), "uri"));
    assert(_uri.is_valid());
    if(!refresh_dns()) assert(false);
}

void httpclient::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);

    bee::rwlock::wrscoped l(_locker);
    _connections.idle.insert(sid);
    advance_all();
}

void httpclient::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);

    bee::rwlock::wrscoped l(_locker);
    _connections.idle.erase(sid);
    if(auto busy_iter = _connections.busy.find(sid); busy_iter != _connections.busy.end())
    {
        local_log("httpclient %s on_del_session, sid %lu found in busy connections, requestid %ld, remove it.", identity(), sid, busy_iter->second);
        _connections.busy.erase(busy_iter);
    }
    advance_all();
}

void httpclient::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httpresponse::TYPE) return;

    if(auto* rsp = static_cast<httpresponse*>(protocol))
    {
        handle_response(rsp);
    }
}

int httpclient::send_request(HTTP_METHOD method, const std::string& path, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
{
    if(path.empty())
    {
        local_log("httpclient %s send_request failed, url is empty.", identity());
        return HTTP_RESULT_INVALID_URL;
    }

    uri request_uri = _uri;
    request_uri.set_path(path);
    return send_request(method, std::move(request_uri), std::move(cbk), timeout, headers, body);
}

int httpclient::send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers, const std::string& body)
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

    start_task(req, std::move(cbk), timeout > 0 ? timeout : _http_task_timeout);

    advance_all();

    return HTTP_RESULT_OK;
}

int httpclient::get(const std::string& path, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_GET, path, std::move(cbk), timeout, std::move(headers));
}

int httpclient::post(const std::string& path, const std::string& body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_POST, path, std::move(cbk), timeout, std::move(headers), body);
}

int httpclient::put(const std::string& path, const std::string& body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_PUT, path, std::move(cbk), timeout, std::move(headers), body);
}

int httpclient::del(const std::string& path, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_DELETE, path, std::move(cbk), timeout, std::move(headers));
}

int httpclient::head(const std::string& path, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_HEAD, path, std::move(cbk), timeout, std::move(headers));
}

int httpclient::options(const std::string& path, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_OPTIONS, path, std::move(cbk), timeout, std::move(headers));
}

int httpclient::patch(const std::string& path, std::string body, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_PATCH, path, std::move(cbk), timeout, std::move(headers), std::move(body));
}

int httpclient::trace(const std::string& path, callback cbk, TIMETYPE timeout, httpprotocol::MAP_TYPE headers)
{
    return send_request(HTTP_METHOD_TRACE, path, std::move(cbk), timeout, std::move(headers));
}

void httpclient::start_task(httprequest* req, callback cbk, TIMETYPE timeout)
{
    HTTP_TASKID taskid = ++_next_http_taskid;
    auto* task = new http_functional_callback(taskid, req, systemtime::get_time() + timeout, std::move(cbk));
    _http_tasks.emplace(taskid, task);
    _connections.pending.emplace_back(taskid);
}

auto httpclient::find_task(HTTP_TASKID taskid) -> http_callback*
{
    auto iter = _http_tasks.find(taskid);
    return iter != _http_tasks.end() ? static_cast<http_callback*>(iter->second) : nullptr;
}

void httpclient::finish_task(http_callback* task)
{
#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
    _http_tasks.erase(task->get_taskid());
}

void httpclient::handle_response(httpresponse* rsp)
{
    const SID sid = rsp->_sid;

    bee::rwlock::wrscoped l(_locker);
    
    auto busy_iter = _connections.busy.find(sid);
    if(busy_iter == _connections.busy.end())
    {
        local_log("httpclient %s handle_response failed, sid %lu not found in busy connections", identity(), sid);
        return;
    }

    HTTP_TASKID taskid = busy_iter->second;
    auto task_iter = _http_tasks.find(taskid);
    if(task_iter == _http_tasks.end())
    {
        _connections.busy.erase(busy_iter);
        recycle_connection(sid, rsp->is_keepalive());
        local_log("httpclient %s handle_response failed, request not found for sid %lu, maybe request is timeout.", identity(), sid);
        return;
    }

    auto* task = static_cast<http_callback*>(task_iter->second);
    ASSERT(task);

    httprequest* req = task->get_request();
    ASSERT(req);

    ostringstream oss;
    rsp->dump(oss);
    local_log("httpclient::handle_response, %s", oss.str().data());

    // 处理响应
    task->set_response(rsp);
    task->set_status(rsp->get_status());
    finish_task(task);

    // 清理状态
    _connections.busy.erase(busy_iter);
    recycle_connection(sid, req->is_keepalive() && rsp->is_keepalive());

    // 如果有等待的请求和空闲连接，继续处理
    advance_all();
}

void httpclient::recycle_connection(SID sid, bool is_keepalive)
{
    if(is_keepalive)
    {
        // 如果响应是长连接，则将连接标记为空闲
        _connections.idle.insert(sid);
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

void httpclient::try_new_connection()
{
    if(!check_connection_count()) return;
    connect();
    local_log("httpclient %s try_new_connection.", identity());
}

bool httpclient::refresh_dns(const std::string& uri_str)
{
    if(!uri_str.empty())
    {
        _uri = uri(uri_str);
    }

    if(!_uri.is_valid())
    {
        local_log("httpclient %s refresh_dns failed, uri_str is invalid.", identity());
        return false;
    }

    _dns.host = _uri.get_host();
    _dns.port = _uri.get_port();
    address* addr = address::lookup_any(_dns.host, _family, _socktype);
    if(!addr)
    {
        local_log("httpclient %s refresh_dns failed, cant find address %s", identity(), _dns.host.data());
        return false;
    }
    set_addr(addr);

    local_log("httpclient %s refresh_dns success, %s --> %s:%d", identity(), uri_str.data(), _dns.host.data(), _dns.port);
    return true;
}

void httpclient::check_timeouts()
{
    TIMETYPE now = systemtime::get_time();

    for(auto iter = _http_tasks.begin(); iter != _http_tasks.end(); )
    {
        if(now >= iter->second->get_timeout()) // 超时处理
        {
            local_log("httpclient %s check_http_task_timeouts, taskid %lu timeout.", identity(), iter->second->get_taskid());
            auto* task = static_cast<http_callback*>(iter->second);
            ASSERT(task);
            task->set_status(HTTP_STATUS_REQUEST_TIMEOUT);
            iter = _http_tasks.erase(iter);
            finish_task(task);
        }
        else
        {
            ++iter;
        }
    }
}

bool httpclient::check_headers(const httprequest* req)
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

void httpclient::advance_all()
{
    check_timeouts();

    while(!_connections.pending.empty() && !_connections.idle.empty())
    {
        // 获取等待的请求和空闲连接
        HTTP_TASKID taskid = _connections.pending.front();
        _connections.pending.pop_front();
        
        const SID sid = *_connections.idle.begin();
        _connections.idle.erase(sid);
        
        auto* task = find_task(taskid);
        ASSERT(task);

        httprequest* req = task->get_request();
        ASSERT(req);

        // 发送请求
        if(session* ses = find_session_nolock(sid))
        {
            // 设置请求的会话ID
            req->init_session(ses);

            // 发送请求
            send_request_nolock(ses, *req);

            // 将连接标记为忙碌
            _connections.busy[sid] = taskid;
        }
        else
        {
            local_log("session_manager %s cant find session %lu on sending protocol", identity(), sid);
        }
    }

    // 如果还有等待的请求但没有空闲连接，尝试创建新连接
    if(!_connections.pending.empty() && _connections.pending.empty())
    {
        try_new_connection();
    }
}

} // namespace bee