#pragma once
#include "httpprotocol.h"
#include "lock.h"
#include "http_task.h"
#include <string>

namespace bee
{

class httpserver;

class servlet : public http_task
{
public:
    servlet(const std::string& name) : _name(name) {}
    virtual ~servlet() = default;
    virtual void run() override;
    virtual int handle(httprequest* req, httpresponse* rsp) = 0;
    virtual servlet* dup() const = 0;
    const std::string& get_name() const { return _name; }

    httpserver* get_manager() const;
    void reply(const std::string& result = ""); // 默认是明文
    void reply(HTTP_CONTENT_TYPE content_type, const std::string& result);
    FORCE_INLINE void reply_html(const std::string& result) { reply(HTTP_CONTENT_TYPE_HTML, result); }
    FORCE_INLINE void reply_css(const std::string& result) { reply(HTTP_CONTENT_TYPE_CSS, result); }
    FORCE_INLINE void reply_xml(const std::string& result) { reply(HTTP_CONTENT_TYPE_XML, result); }
    FORCE_INLINE void reply_json(const std::string& result) { reply(HTTP_CONTENT_TYPE_JSON, result); }
    FORCE_INLINE void reply_javascript(const std::string& result) { reply(HTTP_CONTENT_TYPE_JS, result); }

protected:
    virtual void on_init();
    virtual void on_finish(HTTP_CONTENT_TYPE content_type, const std::string& result);
    virtual void on_error(int retcode);
    virtual void on_timeout();

    std::string get_retcode_message(int retcode);

protected:
    friend class httpserver;
    std::string _name;
};

class function_servlet : public servlet
{
public:
    using callback = std::function<int(httprequest*, httpresponse*)>;
    function_servlet(callback cbk);
    virtual int handle(httprequest* req, httpresponse* rsp) override;
    virtual function_servlet* dup() const override;

private:
    callback _cbk;
};

class servlet_dispatcher : public servlet
{
public:
    servlet_dispatcher();
    ~servlet_dispatcher();
    virtual int handle(httprequest* req, httpresponse* rsp) override;
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
    virtual int handle(httprequest* req, httpresponse* rsp) override;
    virtual not_found_servlet* dup() const override;

private:
    std::string _name;
    std::string _content;
};

} // namespace bee