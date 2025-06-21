#pragma once
#include "http_task.h"

namespace bee
{

class http_callback : public http_task
{
public:
    http_callback(HTTP_TASKID taskid, httprequest* req, TIMETYPE timeout)
        : http_task(taskid, req, timeout) {}

    FORCE_INLINE void set_status(int status) { _status = status; }

protected:
    int _status = HTTP_STATUS_OK;
};

class http_functional_callback : public http_callback
{
public:
    http_functional_callback(HTTP_TASKID taskid, httprequest* req, TIMETYPE timeout, http_callback_func cbk)
        : http_callback(taskid, req, timeout), _callback(std::move(cbk)) {}

    virtual void run() override
    {
        if(_callback)
        {
            _callback(_status, _req, _rsp);
        }
    }

protected:
    http_callback_func _callback;
};

} // namespace bee