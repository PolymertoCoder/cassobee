#include "servlet.h"
#include "http.h"
#include "httpprotocol.h"
#include "httpserver.h"
#include "httpsession_manager.h"
#include "glog.h"
#include <fnmatch.h>

namespace bee
{

void servlet::run()
{
    on_init();
    if(int retcode = handle(_req, _rsp))
    {
        on_error(retcode);
        local_log("servlet %s handle failed, ret=%d.", _name.data(), retcode);
    }
}

httpserver* servlet::get_manager() const
{
    return static_cast<httpserver*>(_req->_manager);
}

void servlet::reply(const std::string& result)
{
    static_cast<httpserver*>(_req->_manager)->reply(_taskid, result);
}

void servlet::on_init()
{
    
}

void servlet::on_error(int retcode)
{
    _rsp->set_status(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    _rsp->set_body(get_retcode_message(HTTP_STATUS_INTERNAL_SERVER_ERROR));

    httpserver* http_server = get_manager();
    if(session* ses = http_server->find_session_nolock(_req->_sid))
    {
        http_server->send_response_nolock(ses, *_rsp);
    }
}

void servlet::on_finish(const std::string& result)
{
    if(result.size()) _rsp->set_body(result);

    httpserver* http_server = get_manager();
    if(session* ses = http_server->find_session_nolock(_req->_sid))
    {
        http_server->send_response_nolock(ses, *_rsp);
    }
}

void servlet::on_timeout()
{
    _rsp->set_status(HTTP_STATUS_SERVICE_UNAVAILABLE);
    _rsp->set_body(get_retcode_message(HTTP_STATUS_SERVICE_UNAVAILABLE));

    httpserver* http_server = get_manager();
    if(session* ses = http_server->find_session_nolock(_req->_sid))
    {
        http_server->send_response_nolock(ses, *_rsp);
    }
}

std::string servlet::get_retcode_message(int retcode)
{
    if(retcode == 0) return "OK";
    if(retcode > 0) // http status
    {
        return http_status_to_string((HTTP_STATUS)retcode);
    }
    else // < 0, http result
    {
        return http_result_to_string((HTTP_RESULT)retcode);
    }
}

function_servlet::function_servlet(callback cbk)
    : servlet("function_servlet"), _cbk(std::move(cbk)) {}

int function_servlet::handle(httprequest* req, httpresponse* rsp)
{
    return _cbk(req, rsp);
}

function_servlet* function_servlet::dup() const
{
    return new function_servlet(_cbk);
}

servlet_dispatcher::servlet_dispatcher()
    : servlet("servlet_dispatcher")
{
    _default = new not_found_servlet("cassobee/1.0.0");
}

servlet_dispatcher::~servlet_dispatcher()
{
    delete _default;
    for(const auto& [_, srv] : _servlets)
    {
        delete srv;
    }
    _servlets.clear();
    for(const auto& [_, srv] : _glob_servlets)
    {
        delete srv;
    }
    _glob_servlets.clear();
}

int servlet_dispatcher::handle(httprequest* req, httpresponse* rsp)
{
    if(servlet* srv = get_matched_servlet(req->get_path()))
    {
        srv->handle(req, rsp);
    }
    else
    {
        _default->handle(req, rsp);
    }
    return 0;
}

servlet_dispatcher* servlet_dispatcher::dup() const
{
    servlet_dispatcher* dispatcher = new servlet_dispatcher();

    rwlock::rdscoped l(_locker);
    dispatcher->_default = _default->dup();
    for(const auto& [uri, srv] : _servlets)
    {
        dispatcher->_servlets[uri] = srv->dup();
    }
    for(const auto& [uri, srv] : _glob_servlets)
    {
        dispatcher->_glob_servlets.emplace_back(uri, srv->dup());
    }
    return dispatcher;
}

void servlet_dispatcher::add_servlet(const std::string& uri, servlet* srv)
{
    rwlock::wrscoped l(_locker);
    _servlets[uri] = srv;
}

void servlet_dispatcher::add_servlet(const std::string& uri, function_servlet::callback cbk)
{
    rwlock::wrscoped l(_locker);
    _servlets[uri] = new function_servlet(std::move(cbk));
}

void servlet_dispatcher::add_glob_servlet(const std::string& uri, servlet* srv)
{
    rwlock::wrscoped l(_locker);
    _glob_servlets.emplace_back(uri, srv);
}

void servlet_dispatcher::add_glob_servlet(const std::string& uri, function_servlet::callback cbk)
{
    rwlock::wrscoped l(_locker);
    _glob_servlets.emplace_back(uri, new function_servlet(std::move(cbk)));
}

void servlet_dispatcher::del_servlet(const std::string& uri)
{
    rwlock::wrscoped l(_locker);
    _servlets.erase(uri);
}

void servlet_dispatcher::del_glob_servlet(const std::string& uri)
{
    rwlock::wrscoped l(_locker);
    std::erase_if(_glob_servlets, [&uri](const auto& pair) {
        return pair.first == uri;
    });
}

servlet* servlet_dispatcher::get_servlet(const std::string& uri)
{
    rwlock::rdscoped l(_locker);
    auto iter = _servlets.find(uri);
    return iter != _servlets.end() ? iter->second->dup() : nullptr;
}

servlet* servlet_dispatcher::get_glob_servlet(const std::string& uri)
{
    rwlock::rdscoped l(_locker);
    auto iter = std::find_if(_glob_servlets.begin(), _glob_servlets.end(),
            [&uri](const auto& pair) { return fnmatch(pair.first.data(), uri.data(), 0) == 0; });
    return iter != _glob_servlets.end() ? iter->second->dup() : nullptr;
}

servlet* servlet_dispatcher::get_matched_servlet(const std::string& uri)
{
    rwlock::rdscoped l(_locker);
    if(auto iter = _servlets.find(uri); iter != _servlets.end())
    {
        return iter->second->dup();
    }
    auto iter = std::find_if(_glob_servlets.begin(), _glob_servlets.end(),
            [&uri](const auto& pair) { return fnmatch(pair.first.data(), uri.data(), 0) == 0; });
    if(iter != _glob_servlets.end())
    {
        return iter->second->dup();
    }
    return nullptr;
}

not_found_servlet::not_found_servlet(const std::string& name)
    : servlet("not_found_servlet"), _name(name)
{
    _content = "<html>"
                    "<head>"
                        "<title>""404 Not Found""</title>"
                    "</head>"
                    "<body>"
                        "<center>"
                            "<h1>""404 Not Found""</h1>"
                        "</center>"
                        "<hr>""<center>" + name + "</center>"
                    "</body>"
                "</html>";
}

int not_found_servlet::handle(httprequest* req, httpresponse* rsp)
{
    rsp->set_status(HTTP_STATUS_NOT_FOUND);
    rsp->set_header("Server", "cassobee/1.0.0");
    rsp->set_header("Content-Type", "txt/html");
    rsp->set_body(_content);
    return 0;
}

not_found_servlet* not_found_servlet::dup() const
{
    not_found_servlet* srv = new not_found_servlet(_name);
    srv->_content = _content;
    return srv;
}

} // namespace bee