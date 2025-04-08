#pragma once
#include "httpprotocol.h"
#include "httpsession.h"
#include "session_manager.h"
#include "types.h"
#include <deque>

namespace bee
{

class httprequest;
class httpresponse;

class httpsession_manager : public session_manager
{
public:
    httpsession_manager();
    virtual ~httpsession_manager();
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    virtual void reconnect() override;

    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;

    FORCE_INLINE bool ssl_enabled() const { return _ssl_ctx != nullptr; }
    FORCE_INLINE SSL_CTX* get_ssl_ctx() const { return _ssl_ctx; }

    static SSL_CTX* create_ssl_context(const std::string& cert_path, const std::string& key_path);
    bool check_headers(const httpprotocol* req);
    static void ocsp_callback(SSL* ssl, void* arg);

    void send_request(SID sid, const httprequest& req, httprequest::callback cbk);
    void send_response(SID sid, const httpresponse& rsp);

protected:
    SSL_CTX* _ssl_ctx = nullptr;
    std::string _cert_path;
    std::string _key_path;

    struct pending_request
    {
        httprequest* req;
        httprequest::callback cbk;
        TIMETYPE timeout;
    };
    std::deque<pending_request> _pending_requests;
};

} // namespace bee