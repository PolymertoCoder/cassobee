#pragma once
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <deque>
#include <string>

namespace bee
{

class uri;
class httprequest;
class httpresponse;
class servlet_dispatcher;

struct http_result
{
    enum class ERR : int
    {
        OK,
        INVALID_URL,
        INVALUE_HOST,
        CONNECT_FAIL,
        SEND_CLOSE_BY_PEER,
        SEND_SOCKET_ERROR,
        TIMEOUT,
        SSL_NOT_ENABLED,
        CREATE_SOCKET_ERROR,
        POOL_GET_CONNECTION_ERROR,
        POOL_INVALID_CONNECTION,
    };

    http_result(ERR _err, httpresponse* _rsp, const std::string& _errmsg)
        : err(_err), rsp(_rsp), err_msg(_errmsg) {}

    ERR err = ERR::OK;
    httpresponse* rsp = nullptr;
    std::string err_msg;
};

class httpsession_manager : public session_manager
{
public:
    httpsession_manager();
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;

    bool check_headers(const httpprotocol* req);
    void check_timeouts();

    void send_request(SID sid, const httprequest& req, httprequest::callback cbk);
    void send_response(SID sid, const httpresponse& rsp);
};

class httpclient_manager : public httpsession_manager
{
public:
    virtual ~httpclient_manager();
    virtual const char* identity() const override { return "httpclient_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;

    http_result send_request(HTTP_METHOD method, const std::string& url, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(HTTP_METHOD method, const uri& uri, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(const httprequest* req, const uri& uri, TIMETYPE timeout/*ms*/);

protected:
    void try_new_connection(const httprequest* req, const uri& uri, TIMETYPE timeout/*ms*/);
    void refresh_dns();

protected:
    struct
    {
        std::string host;
        int32_t port;
    } _dns;
    int _max_requests = 0;
    int _request_timeout = 0;
    std::set<SID> _free_connections; // 已连接但闲置中的链接
    std::set<SID> _busy_connections; // 已发送请求等待回应的链接
    std::deque<httprequest*> _pending_requests; // 未发送的http请求
};

class httpserver_manager : public httpsession_manager
{
public:
    virtual ~httpserver_manager();
    virtual const char* identity() const override { return "httpserver_manager"; }

    virtual void init() override;

    void send_response();

protected:
    servlet_dispatcher* _dispatcher = nullptr;
};

} // namespace bee