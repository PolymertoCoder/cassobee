#pragma once
#include "httpprotocol.h"
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <deque>

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
    virtual ~httpsession_manager();
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    virtual void reconnect() override;

    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;

    bool check_headers(const httpprotocol* req);

    void send_request(SID sid, const httprequest& req, httprequest::callback cbk);
    void send_response(SID sid, const httpresponse& rsp);

protected:
    int _max_requests = 0;
    int _request_timeout = 0;
    struct pending_request
    {
        httprequest* req;
        httprequest::callback cbk;
        TIMETYPE timeout;
    };
    std::deque<pending_request> _pending_requests;
    servlet_dispatcher* _dispatcher = nullptr;
};

class httpclient_manager : public httpsession_manager
{
public:
    http_result do_get(const std::string& url, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result do_get(const uri& uri, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");

    http_result do_post(const std::string& url, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result do_post(const uri& uri, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");

    http_result send_request(HTTP_METHOD method, const std::string& url, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(HTTP_METHOD method, const uri& uri, TIMETYPE timeout/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    http_result send_request(const httprequest& req, const uri& uri, TIMETYPE timeout/*ms*/);
};

class httpserver_manager : public httpsession_manager
{
public:

};

} // namespace bee