#pragma once
#include "httpsession_manager.h"
#include "uri.h"
#include <deque>

namespace bee
{

class httpclient : public httpsession_manager
{
public:
    using callback  = http_callback_func;
    virtual const char* identity() const override { return "httpclient"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    virtual size_t thread_group_idx() override { return 0; }
    virtual void handle_protocol(httpprotocol* protocol) override;

    int send_request(HTTP_METHOD method, const std::string& url, callback cbk, TIMETYPE timeout = 0, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    int send_request(HTTP_METHOD method, const uri& uri, callback cbk, TIMETYPE timeout = 0, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");

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
    void start_task(httprequest* req, callback cbk, TIMETYPE timeout);
    auto find_task(HTTP_TASKID taskid) -> http_callback*;
    void finish_task(http_callback* task);

    void handle_response(httpresponse* rsp);
    void recycle_connection(SID sid, bool is_keepalive);
    void try_new_connection();
    bool refresh_dns(const std::string& uri_str = {});
    void check_timeouts();
    bool check_headers(const httprequest* req);
    void advance_all();

protected:
    struct
    {
        std::string host;
        int32_t port = 0;
    } _dns;

    uri _uri;

    struct connection_pool
    {
        std::set<SID> idle; // 空闲的连接
        std::map<SID, HTTP_TASKID> busy; // 已发送请求，等待回应中的连接
        std::deque<HTTP_TASKID> pending;  // 待发送的请求
    } _connections;
};

} // namespace bee