#pragma once
#include "httpsession_manager.h"

namespace bee
{

class servlet_dispatcher;
class servlet;

class httpserver : public httpsession_manager
{
public:
    virtual ~httpserver();
    virtual const char* identity() const override { return "httpserver"; }

    virtual void init() override;
    virtual size_t thread_group_idx() override { return 0; }
    virtual void handle_protocol(httpprotocol* protocol) override;

    void reply(HTTP_TASKID taskid, const std::string& result = "");
    void reply(HTTP_TASKID taskid, HTTP_CONTENT_TYPE content_type = HTTP_CONTENT_TYPE_PLAIN, const std::string& result = "");

protected:
    void start_task(httprequest* req, httpresponse* rsp);
    auto find_task(HTTP_TASKID taskid) -> servlet*;
    void finish_task(HTTP_TASKID taskid, HTTP_CONTENT_TYPE content_type = HTTP_CONTENT_TYPE_PLAIN, const std::string& result = "");

    void handle_request(httprequest* req);
    void check_timeouts();

protected:
    servlet_dispatcher* _dispatcher = nullptr;
};

} // namespace bee