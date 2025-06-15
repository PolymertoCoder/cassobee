#include "servlet.h"
#include "http.h"
#include "httpprotocol.h"
#include <fnmatch.h>

namespace bee
{

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