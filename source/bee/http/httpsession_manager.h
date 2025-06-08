#pragma once
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <list>
#include <string>
#include <unordered_map>

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

    virtual void init() override;
    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;

    bool check_headers(const httpprotocol* req);

    void send_request(SID sid, const httprequest& req, httprequest::callback cbk);
    void send_response(SID sid, const httpresponse& rsp);
};

class httpclient_manager : public httpsession_manager
{
public:
    using REQUESTID = httprequest::REQUESTID;
    using callback  = httprequest::callback;

    virtual ~httpclient_manager();
    virtual const char* identity() const override { return "httpclient_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;

    http_result send_request(HTTP_METHOD method, const std::string& url, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(HTTP_METHOD method, const uri& uri, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(httprequest* req, const uri& uri, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/);

    httprequest* find_httprequest(REQUESTID requestid);

protected:
    auto get_new_requestid() -> REQUESTID;
    void try_new_connection(httprequest* req, const uri& uri, TIMETYPE timeout/*ms*/);
    void refresh_dns();

protected:
    struct
    {
        std::string host;
        int32_t port = 0;
    } _dns;
    size_t _max_requests = 0;
    size_t _request_timeout = 0;
    std::set<SID> _idle_connections; // 还未发送请求的连接
    std::map<SID, REQUESTID> _busy_connections; // 已发送请求，等待回应中的连接
    std::list<REQUESTID> _waiting_requests; // 未发送的请求
    std::unordered_map<REQUESTID, httprequest*> _requests_cache; // 未完成的请求，包括已发送和未发送的
};

class httpserver_manager : public httpsession_manager
{
public:
    virtual ~httpserver_manager();
    virtual const char* identity() const override { return "httpserver_manager"; }

    virtual void init() override;

    FORCE_INLINE servlet_dispatcher* get_dispatcher() { return _dispatcher; }

    http_result send_response();

protected:
    servlet_dispatcher* _dispatcher = nullptr;
};

} // namespace bee