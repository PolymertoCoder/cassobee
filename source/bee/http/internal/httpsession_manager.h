#pragma once
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <unordered_map>

namespace bee
{

class httprequest;
class httpresponse;
class http_task;

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

protected:
    void send_request_nolock(session* ses, const httprequest& req);
    void send_response_nolock(session* ses, const httpresponse& rsp);

protected:
    friend class servlet;
    HTTP_TASKID _next_http_taskid = 0;
    TIMETYPE _http_task_timeout = 0; // HTTP任务超时时间
    std::unordered_map<HTTP_TASKID, http_task*> _http_tasks; // HTTP任务缓存
};

} // namespace bee