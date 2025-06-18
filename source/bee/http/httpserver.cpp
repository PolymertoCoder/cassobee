#include "httpserver.h"
#include "config_servlet.h"
#include "http_task.h"
#include "glog.h"
#include "servlet.h"
#include "systemtime.h"
#ifdef _REENTRANT
#include "threadpool.h"
#endif

namespace bee
{

httpserver::~httpserver()
{
    delete _dispatcher;
}

void httpserver::init()
{
    httpsession_manager::init();

    _dispatcher = new servlet_dispatcher;
    _dispatcher->add_servlet("/_/config", new config_servlet);
}

void httpserver::handle_protocol(httpprotocol* protocol)
{
    if(!protocol) return;
    if(protocol->get_type() != httprequest::TYPE) return;

    if(auto* req = dynamic_cast<httprequest*>(protocol))
    {
        handle_request(req);
    }
}

void httpserver::start_task(httprequest* req, httpresponse* rsp)
{
    bee::rwlock::wrscoped l(_locker);
    HTTP_TASKID taskid = ++_next_http_taskid;
    auto* task = new http_servlet_task(taskid, req, rsp, systemtime::get_time() + _http_task_timeout);    
    _http_tasks.emplace(taskid, task);

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

auto httpserver::find_task(HTTP_TASKID taskid) -> http_servlet_task*
{
    auto iter = _http_tasks.find(taskid);
    return iter != _http_tasks.end() ? static_cast<http_servlet_task*>(iter->second) : nullptr;
}

void httpserver::finish_task(HTTP_TASKID taskid)
{
    auto iter = _http_tasks.find(taskid);
    if(iter == _http_tasks.end())
    {
        local_log("httpclient %s finish_http_task failed, taskid %lu not found.", identity(), taskid);
        return;
    }
    auto* task = iter->second;
    _http_tasks.erase(iter);
    task->destroy();
}

void httpserver::handle_request(httprequest* req)
{
    httpresponse* rsp = httpprotocol::get_response();

    // 初始化响应基本信息
    rsp->set_version(req->get_version());
    rsp->set_header("server", identity());

    start_task(req, rsp);
}

void httpserver::start_http_task_nolock(httprequest* req, httpresponse* rsp)
{
    HTTP_TASKID taskid = ++_next_http_taskid;
    auto* task = new http_servlet_task(taskid, req, rsp, systemtime::get_time() + _http_task_timeout);
    _http_tasks.emplace(taskid, task);
}

void httpserver::finish_http_task_nolock(HTTP_TASKID taskid)
{
    auto iter = _http_tasks.find(taskid);
    if(iter == _http_tasks.end())
    {
        local_log("httpserver %s finish_http_task failed, taskid %lu not found.", identity(), taskid);
        return;
    }
    auto* task = iter->second;
    _http_tasks.erase(iter);

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

void httpserver::check_timeouts()
{
    TIMETYPE now = systemtime::get_time();

    for(auto iter = _http_tasks.begin(); iter != _http_tasks.end();)
    {
        if(now >= iter->second->get_timeout())
        {
            local_log("httpserver %s check_http_task_timeouts, taskid %lu timeout.", identity(), iter->second->get_taskid());
            iter->second->run();
            iter = _http_tasks.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

} // namespace bee