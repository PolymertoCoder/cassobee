#include "httpserver.h"
#include "config_servlet.h"
#include "http_task.h"
#include "glog.h"
#include "httpprotocol.h"
#include "log.h"
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
    reply(taskid, HTTP_CONTENT_TYPE_PLAIN, result);
}

void httpserver::reply(HTTP_TASKID taskid, std::string&& result)
{
    reply(taskid, HTTP_CONTENT_TYPE_PLAIN, std::move(result));
}

void httpserver::reply(HTTP_TASKID taskid, HTTP_CONTENT_TYPE content_type, const std::string& result)
{
    finish_task(taskid, content_type, result);
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

void httpserver::finish_task(HTTP_TASKID taskid, HTTP_CONTENT_TYPE content_type, const std::string& result)
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
    task->on_finish(content_type, result);
}

void httpserver::handle_request(httprequest* req)
{
    httpresponse* rsp = httpprotocol::get_response();

    // 初始化响应基本信息
    rsp->set_version(req->get_version());
    rsp->set_header("server", identity());

    constexpr static const char* cookie_keys[] = {
        "sessionid", "token", "username", "password",
        "email", "phone", "address", "city", "state"
    };
    for (const std::string& key : cookie_keys)
    {
        if(req->has_cookie(key))
        {
            rsp->set_cookie(key, req->get_cookie(key), /*expire*/ 3600, "/", "", true, true);
        }
    }

    ostringstream oss;
    req->dump(oss);
    local_log("httpserver::handle_request, %s", oss.str().data());

    start_task(req, rsp);
}

void httpserver::check_timeouts()
{
    TIMETYPE now = systemtime::get_time();

    for(auto iter = _http_tasks.begin(); iter != _http_tasks.end();)
    {
        if(now >= iter->second->get_timeout())
        {
            auto* task = static_cast<servlet*>(iter->second);
            local_log("httpserver %s check_http_task_timeouts, taskid %lu timeout.", identity(), iter->second->get_taskid());
            task->on_timeout();
            iter = _http_tasks.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

} // namespace bee