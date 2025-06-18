#pragma once
#include "httpsession_manager.h"

namespace bee
{

class httpserver : public httpsession_manager
{
public:
    virtual ~httpserver();
    virtual const char* identity() const override { return "httpserver"; }

    virtual void init() override;
    virtual size_t thread_group_idx() override { return 0; }
    virtual void handle_protocol(httpprotocol* protocol) override;

    FORCE_INLINE servlet_dispatcher* get_dispatcher() { return _dispatcher; }

protected:
    void start_task(httprequest* req, httpresponse* rsp);
    auto find_task(HTTP_TASKID taskid) -> http_servlet_task*;
    void finish_task(HTTP_TASKID taskid);

    void handle_request(httprequest* req);
    void start_http_task_nolock(httprequest* req, httpresponse* rsp);
    void finish_http_task_nolock(HTTP_TASKID taskid);
    void check_timeouts();

protected:
    servlet_dispatcher* _dispatcher = nullptr;
};

} // namespace bee