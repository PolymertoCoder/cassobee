#pragma once
#include "runnable.h"
#include "httpprotocol.h"
#include "httpsession_manager.h"
#include "servlet.h"

namespace bee
{

class http_task : public runnable
{
public:
    http_task(HTTP_TASKID taskid, httprequest* req) : _taskid(taskid), _req(req) {}

    FORCE_INLINE HTTP_TASKID get_taskid() const { return _taskid; }
    FORCE_INLINE void set_request(httprequest* req) { _req = req; }
    FORCE_INLINE void set_response(httpresponse* rsp) { _rsp = rsp; }

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
    httprequest*  _req  = nullptr;
    httpresponse* _rsp  = nullptr;
};

class http_callback : public http_task
{
public:
    http_callback(HTTP_TASKID taskid, httprequest* req)
        : http_task(taskid, req) {}

    FORCE_INLINE void set_status(int status) { _status = status; }

    virtual void run() = 0;

protected:
    int _status = HTTP_STATUS_OK;
};

class http_functional_callback : public http_callback
{
public:
    http_functional_callback(HTTP_TASKID taskid, httprequest* req, http_callback_func cbk)
        : http_callback(taskid, req), _callback(std::move(cbk)) {}

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
        : http_task(taskid, req), _timeout(timeout) {}

    FORCE_INLINE TIMETYPE get_timeout() const { return _timeout; }

    virtual void run() override
    {
        if(!_req || !_rsp) return;

        auto* httpserver = static_cast<httpserver_manager*>(_req->_manager);
        if(!httpserver) return;
        if(auto* dispatcher = httpserver->get_dispatcher())
        {
            dispatcher->handle(_req, _rsp);
        }
    }

protected:
    TIMETYPE _timeout = 0;
};

} // namesapce bee