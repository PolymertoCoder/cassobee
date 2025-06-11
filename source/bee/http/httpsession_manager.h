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
class http_callback;

class httpsession_manager : public session_manager
{
public:
    httpsession_manager();
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual void init() override;
    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;
    virtual void handle_protocol(httpprotocol* protocol) = 0;

    bool check_headers(const httpprotocol* req);

    void send_request_nolock(session* ses, const httprequest& req);
    void send_response_nolock(session* ses, const httpresponse& rsp);
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
    virtual void handle_protocol(httpprotocol* protocol) override;

    int send_request(HTTP_METHOD method, const std::string& url, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    int send_request(HTTP_METHOD method, const uri& uri, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/, const httpprotocol::MAP_TYPE& headers = {}, const std::string& body = "");
    int send_request(httprequest* req, const uri& uri, callback cbk = {}, TIMETYPE timeout = 30000/*ms*/);

    httprequest* find_httprequest(REQUESTID requestid);
    httprequest* find_httprequest_by_sid(SID sid);

    void handle_response(int result, httpresponse* rsp);
    void check_timeouts();

protected:
    auto get_new_requestid() -> REQUESTID;
    void try_new_connection();
    bool refresh_dns(const std::string& uri_str = {});
    void advance_all();

protected:
    struct
    {
        std::string host;
        int32_t port = 0;
    } _dns;
    uri _uri;
    size_t _max_requests = 0;
    size_t _request_timeout = 0;
    std::set<SID> _idle_connections; // 还未发送请求的连接
    std::map<SID, REQUESTID> _busy_connections; // 已发送请求，等待回应中的连接
    std::deque<REQUESTID> _waiting_requests; // 未发送的请求
    std::unordered_map<REQUESTID, httprequest*> _requests_cache; // 未完成的请求，包括已发送和未发送的
};

class httpserver_manager : public httpsession_manager
{
public:
    virtual ~httpserver_manager();
    virtual const char* identity() const override { return "httpserver_manager"; }

    virtual void init() override;
    virtual void handle_protocol(httpprotocol* protocol) override;

    FORCE_INLINE servlet_dispatcher* get_dispatcher() { return _dispatcher; }

    int send_response();

protected:
    servlet_dispatcher* _dispatcher = nullptr;
};

} // namespace bee