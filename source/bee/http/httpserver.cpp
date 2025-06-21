#include "httpserver.h"
#include "config_servlet.h"
#include "http_task.h"
#include "glog.h"
#include "httpprotocol.h"
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

void httpserver::reply(HTTP_TASKID taskid, const std::string& result)
{
    finish_task(taskid, result);
}

void httpserver::start_task(httprequest* req, httpresponse* rsp)
{
    bee::rwlock::wrscoped l(_locker);
    servlet* task = _dispatcher->get_matched_servlet(req->get_path());
    if(!task)
    {
        local_log("httpserver %s cant find %s servlet.", identity(), req->get_path().data());
        return;
    }

    HTTP_TASKID taskid = ++_next_http_taskid;
    task->set_taskid(taskid);
    task->set_timeout(systemtime::get_time() + _http_task_timeout);
    task->set_request(req);
    task->set_response(rsp);
    _http_tasks.emplace(taskid, task);

#ifdef _REENTRANT
    threadpool::get_instance()->add_task(thread_group_idx(), task);
#else
    task->run();
    task->destroy();
#endif
}

auto httpserver::find_task(HTTP_TASKID taskid) -> servlet*
{
    bee::rwlock::wrscoped l(_locker);
    auto iter = _http_tasks.find(taskid);
    return iter != _http_tasks.end() ? static_cast<servlet*>(iter->second) : nullptr;
}

void httpserver::finish_task(HTTP_TASKID taskid, const std::string& result)
{
    bee::rwlock::wrscoped l(_locker);
    auto iter = _http_tasks.find(taskid);
    if(iter == _http_tasks.end())
    {
        local_log("httpclient %s finish_http_task failed, taskid %lu not found.", identity(), taskid);
        return;
    }
    servlet* task = static_cast<servlet*>(iter->second);
    _http_tasks.erase(iter);
    task->on_finish(result);
}

void httpserver::handle_request(httprequest* req)
{
    httpresponse* rsp = httpprotocol::get_response();

    // 初始化响应基本信息
    rsp->set_version(req->get_version());
    rsp->set_header("server", identity());

    start_task(req, rsp);
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