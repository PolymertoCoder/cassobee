#pragma once
#include "runnable.h"
#include "httpprotocol.h"
#include "httpserver.h"
#include "servlet.h"

namespace bee
{

class http_task : public runnable
{
public:
    http_task(HTTP_TASKID taskid, httprequest* req, TIMETYPE timeout)
        : _taskid(taskid), _timeout(timeout), _req(req) {}

    FORCE_INLINE HTTP_TASKID get_taskid() const { return _taskid; }
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

// 处理请求的task
class http_servlet_task : public http_task
{
public:
    http_servlet_task(HTTP_TASKID taskid, httprequest* req, httpresponse* rsp, TIMETYPE timeout)
        : http_task(taskid, req, timeout) { _rsp = rsp; }

    virtual void run() override
    {
        if(!_req || !_rsp) return;

        auto* server = static_cast<httpserver*>(_req->_manager);
        if(!server) return;

        if(auto* dispatcher = server->get_dispatcher())
        {
            dispatcher->handle(_req, _rsp);
        }
    }
};

} // namesapce bee