#pragma once
#include "httpsession.h"
#include "session_manager.h"

namespace bee
{

class httpsession_manager : public session_manager
{
public:
    httpsession_manager() = default;
    virtual ~httpsession_manager();
    virtual const char* identity() const override { return "httpsession_manager"; }

    virtual void init() override;
    virtual void on_add_session(SID sid) override;
    virtual void on_del_session(SID sid) override;
    virtual void reconnect() override;

    virtual httpsession* create_session() override;
    virtual httpsession* find_session(SID sid) override;

protected:
    SSL_CTX* create_ssl_context(const std::string& cert_path, const std::string& key_path);
    void check_keepalive();
    bool validate_headers(const httpprotocol* req);

protected:
    SSL_CTX* _ssl_ctx = nullptr;
};


} // namespace bee