#pragma once
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include "uri.h"
#include <deque>
#include <string>
#include <unordered_map>

namespace bee
{

class uri;
class httprequest;
class httpresponse;
class servlet_dispatcher;
class http_task;
class http_callback_task;
class http_servlet_task;

class httpsession_manager : public session_manager
{
public:
    httpsession_manager() = default;
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual void init() override;
    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;
    virtual size_t thread_group_idx() = 0;
    virtual void handle_protocol(httpprotocol* protocol) = 0;

    bool check_headers(const httpprotocol* req);

    void send_request_nolock(session* ses, const httprequest& req);
    void send_response_nolock(session* ses, const httpresponse& rsp);

protected:
    HTTP_TASKID _next_http_taskid = 0;
};

class httpclient_manager : public httpsession_manager
{
public:
    using REQUESTID = httprequest::REQUESTID;
    using callback  = httprequest::callback;

    virtual ~httpclient_manager() override;
    virtual const char* identity() const override { return "httpclient_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    virtual size_t thread_group_idx() override { return 0; }
    virtual void handle_protocol(httpprotocol* protocol) override;

    int send_request(HTTP_METHOD method, const std::string& url, callback cbk, TIMETYPE timeout = 0, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    int send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout = 0, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    int send_request(httprequest* req, callback cbk, TIMETYPE timeout = 0);

    // GET请求通常用于获取资源
    int get(const std::string& url, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // POST请求通常用于提交数据
    int post(const std::string& url, const std::string& body, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // PUT请求通常用于更新资源
    int put(const std::string& url, const std::string& body, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // DELETE请求通常用于删除资源
    int del(const std::string& url, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // HEAD请求通常用于获取资源的元信息
    int head(const std::string& url, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // OPTIONS请求通常用于获取服务器支持的HTTP方法
    int options(const std::string& url, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // PATCH请求通常用于对资源进行部分修改
    int patch(const std::string& url, std::string body, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

    // TRACE请求通常用于诊断目的，回显服务器收到的请求
    int trace(const std::string& url, callback cbk, TIMETYPE timeout = 0, httpprotocol::MAP_TYPE headers = {});

protected:
    void add_http_task(int status, httprequest* req, httpresponse* rsp);
    void handle_response(httpresponse* rsp);
    void recycle_connection(SID sid, bool is_keepalive);
    void try_new_connection();
    bool refresh_dns(const std::string& uri_str = {});
    void check_request_timeouts();
    void advance_all();

protected:
    struct
    {
        std::string host;
        int32_t port = 0;
    } _dns;
    uri _uri;
    size_t _request_timeout = 0;
    REQUESTID _next_requestid = 0; // 下一个请求ID
    std::set<SID> _idle_connections; // 空闲的连接
    std::map<SID, REQUESTID> _busy_connections; // 已发送请求，等待回应中的连接
    std::deque<REQUESTID> _waiting_requests; // 待发送的请求
    std::multimap<TIMETYPE, REQUESTID> _request_timeouts; // 请求超时列表
    std::unordered_map<REQUESTID, httprequest*> _requests_cache; // 未完成的请求，包括已发送和未发送的
};

class httpserver_manager : public httpsession_manager
{
public:
    virtual ~httpserver_manager();
    virtual const char* identity() const override { return "httpserver_manager"; }

    virtual void init() override;
    virtual size_t thread_group_idx() override { return 0; }
    virtual void handle_protocol(httpprotocol* protocol) override;

    FORCE_INLINE servlet_dispatcher* get_dispatcher() { return _dispatcher; }

    void start_http_task(httprequest* req, httpresponse* rsp);
    void finish_http_task(HTTP_TASKID taskid);
    void send_response(SID sid, httpresponse* rsp);

protected:
    void start_http_task_nolock(httprequest* req, httpresponse* rsp);
    void finish_http_task_nolock(HTTP_TASKID taskid);
    void check_http_task_timeouts();
    void handle_request(httprequest* req);

protected:
    servlet_dispatcher* _dispatcher = nullptr;
    TIMETYPE _http_task_timeout = 0; // HTTP任务超时时间
    std::unordered_map<HTTP_TASKID, http_servlet_task*> _http_tasks; // HTTP任务缓存
};

} // namespace bee