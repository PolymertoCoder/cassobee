#pragma once
#include "httpprotocol.h"
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <deque>

namespace bee
{

class httprequest;
class httpresponse;
class servlet_dispatcher;

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

} // namespace bee