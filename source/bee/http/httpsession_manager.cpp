#include "httpsession_manager.h"
#include "reactor.h"
#include "config.h"
#include "systemtime.h"
#include "httpprotocol.h"
#include <openssl/err.h>
#include <print>

namespace bee
{

httpsession_manager::~httpsession_manager()
{
    if (_ssl_ctx)
    {
        SSL_CTX_free(_ssl_ctx);
    }
}

void httpsession_manager::init()
{
    session_manager::init();

    auto cfg = config::get_instance();
    if (cfg->get<bool>(identity(), "ssl_enabled", false))
    {
        std::string cert_path = cfg->get(identity(), "cert_file");
        std::string key_path = cfg->get(identity(), "key_file");

        if (cert_path.empty() || key_path.empty())
        {
            throw std::runtime_error("SSL enabled but cert/key not configured");
        }

        _ssl_ctx = create_ssl_context(cert_path, key_path);
        if (!_ssl_ctx)
        {
            throw std::runtime_error("Failed to initialize SSL context");
        }
    }
}

void httpsession_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);

    // 动态保活配置
    auto cfg = config::get_instance();
    int keepalive_timeout = cfg->get<int>(identity(), "keepalive_timeout", 30000);
    int max_requests = cfg->get<int>(identity(), "max_requests", 100);

    add_timer(keepalive_timeout, [this, sid, max_requests, keepalive_timeout]()
    {
        if(auto s = find_session(sid)) {
            // 双重检查避免竞态条件
            bee::rwlock::wrscoped l(s->_locker);
            if(s->_state == SESSION_STATE_ACTIVE) {
                if(s->_request_count >= max_requests || 
                  (s->_last_active + keepalive_timeout) < systemtime::get_millseconds()) {
                    s->close();
                    return false;
                }
                return true;
            }
        }
        return false;
    });
}

void httpsession_manager::on_del_session(SID sid)
{
    session_manager::on_del_session(sid);
}

void httpsession_manager::reconnect()
{

}

httpsession* httpsession_manager::create_session()
{
    auto ses = new httpsession(this);
    if (_ssl_ctx)
    {
        ses->_ssl = SSL_new(_ssl_ctx);
        if (!ses->_ssl)
        {
            delete ses;
            throw std::runtime_error("Failed to create SSL object");
        }
    }
    return ses;
}

httpsession* httpsession_manager::find_session(SID sid)
{
    bee::rwlock::rdscoped l(_locker);
    auto iter = _sessions.find(sid);
    return iter != _sessions.end() ? dynamic_cast<httpsession*>(iter->second) : nullptr;
}

SSL_CTX* httpsession_manager::create_ssl_context(const std::string& cert_path, const std::string& key_path)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        SSL_CTX_free(ctx);
        return nullptr;
    }

    return ctx;
}

// 智能连接保活
void httpsession_manager::check_keepalive()
{
    const TIMETYPE now = systemtime::get_millseconds();
    auto cfg = config::get_instance();
    const TIMETYPE timeout = cfg->get<TIMETYPE>(identity(), "keepalive_timeout", 30000);
    const size_t max_requests = cfg->get<size_t>(identity(), "max_requests", 1000);

    bee::rwlock::wrscoped l(_locker);
    for(auto it = _sessions.begin(); it != _sessions.end();) {
        httpsession* ses = dynamic_cast<httpsession*>(it->second);
        bool should_close = 
            (now - ses->_last_active > timeout) ||
            (ses->_request_count >= max_requests) ||
            (ses->_writeos.size() > 10 * 1024 * 1024); // 10MB写缓冲限制

        if(should_close) {
            std::println("Closing session %lu (requests:%zu buffer:%zuMB)", 
                it->first, ses->_request_count, ses->_writeos.size()/(1024*1024));
            delete ses;
            it = _sessions.erase(it);
        } else {
            ++it;
        }
    }
}

// 请求头安全检查
bool httpsession_manager::validate_headers(const httpprotocol* req)
{
    // 检查Host头
    if (!req->has_header("Host")) {
        return false;
    }

    // 限制头数量
    constexpr size_t MAX_HEADERS = 64;
    if (req->header_count() > MAX_HEADERS) {
        return false;
    }

    // 检查头字段合法性
    static const std::set<std::string> ALLOWED_HEADERS = {
        "Host", "User-Agent", "Accept", "Content-Length", 
        "Content-Type", "Connection"
    };

    for (const auto& [k, v] : req->headers()) {
        // 检查字段名合法性
        if (k.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos) {
            return false;
        }
        
        // 白名单检查
        if (!ALLOWED_HEADERS.contains(k)) {
            return false;
        }
    }
    return true;
}


} // namespace bee