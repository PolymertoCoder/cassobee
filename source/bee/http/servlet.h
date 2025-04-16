#pragma once
#include "httpprotocol.h"
#include "httpsession.h"
#include <string>

namespace bee
{

class servlet
{
public:
    servlet(const std::string& name) : _name(name) {}
    virtual ~servlet() = default;
    virtual int handle(httprequest* req, httpresponse* rsp, httpsession* ses) = 0;
    virtual servlet* dup() const = 0;
    const std::string& get_name() const { return _name; }

private:
    std::string _name;
};

class function_servlet : public servlet
{
public:
    using callback = std::function<int(httprequest*, httpresponse*, httpsession*)>;
    function_servlet(callback cbk);
    virtual int handle(httprequest* req, httpresponse* rsp, httpsession* ses) override;
    virtual function_servlet* dup() const override;

private:
    callback _cbk;
};

class servlet_dispatcher : public servlet
{
public:
    servlet_dispatcher();
    ~servlet_dispatcher();
    virtual int handle(httprequest* req, httpresponse* rsp, httpsession* ses) override;
    virtual servlet_dispatcher* dup() const override;

    void add_servlet(const std::string& uri, servlet* srv);
    void add_servlet(const std::string& uri, function_servlet::callback cbk);
    void add_glob_servlet(const std::string& uri, servlet* srv);
    void add_glob_servlet(const std::string& uri, function_servlet::callback cbk);

    void del_servlet(const std::string& uri);
    void del_glob_servlet(const std::string& uri);

    servlet* get_default() const { return _default; }
    void set_default(servlet* srv) { _default = srv; }

    servlet* get_servlet(const std::string& uri);
    servlet* get_glob_servlet(const std::string& uri);
    servlet* get_matched_servlet(const std::string& uri);

private:
    mutable rwlock _locker;
    servlet* _default = nullptr;
    std::unordered_map<std::string, servlet*>     _servlets;
    std::vector<std::pair<std::string, servlet*>> _glob_servlets;
};

class not_found_servlet : public servlet
{
public:
    not_found_servlet(const std::string& name);
    virtual int handle(httprequest* req, httpresponse* rsp, httpsession* ses) override;
    virtual not_found_servlet* dup() const override;

private:
    std::string _name;
    std::string _content;
};

} // namespace bee