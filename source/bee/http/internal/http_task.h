#pragma once
#include "httpprotocol.h"
#include "runnable.h"

namespace bee
{

class http_task : public runnable
{
public:
    http_task() = default; // for servlet construct
    http_task(HTTP_TASKID taskid, httprequest* req, TIMETYPE timeout)
        : _taskid(taskid), _timeout(timeout), _req(req) {}

    FORCE_INLINE void set_taskid(HTTP_TASKID taskid) { _taskid = taskid; }
    FORCE_INLINE HTTP_TASKID get_taskid() const { return _taskid; }
    FORCE_INLINE void set_timeout(TIMETYPE timeout) { _timeout = timeout; }
    FORCE_INLINE TIMETYPE get_timeout() const { return _timeout; }
    FORCE_INLINE void set_request(httprequest* req) { _req = req; }
    FORCE_INLINE httprequest* get_request() const { return _req; }
    FORCE_INLINE void set_response(httpresponse* rsp) { _rsp = rsp; }
    FORCE_INLINE httpresponse* get_response() const { return _rsp; }

    virtual void destroy() override
    {
        delete _req;
        _req =  nullptr;
        delete _rsp;
        _rsp = nullptr;
        runnable::destroy();
    }

protected:
    HTTP_TASKID _taskid  = 0;
    TIMETYPE    _timeout = 0;
    httprequest*  _req  = nullptr;
    httpresponse* _rsp  = nullptr;
};

} // namesapce bee