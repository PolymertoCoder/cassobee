#include "httpsession_manager.h"
#include "reactor.h"
#include "config.h"
#include "session_manager.h"
#include "systemtime.h"
#include "httpprotocol.h"
#include <openssl/err.h>
#include <print>

namespace bee
{

httpsession_manager::httpsession_manager()
    : session_manager()
{
    _session_type = SESSION_TYPE_HTTP;
}
    
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
        _cert_path = cfg->get(identity(), "cert_file");
        _key_path = cfg->get(identity(), "key_file");

        if (_cert_path.empty() || _key_path.empty())
        {
            throw std::runtime_error("SSL enabled but cert/key not configured");
        }

        _ssl_ctx = create_ssl_context(_cert_path, _key_path);
        if (!_ssl_ctx)
        {
            throw std::runtime_error("Failed to initialize SSL context");
        }
        std::println("SSL context initialized for %s", identity());
    }
    else
    {
        std::println("SSL is disabled for %s", identity());
    }
}

void httpsession_manager::on_add_session(SID sid)
{
    session_manager::on_add_session(sid);
    // TODO 动态保活配置
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
    ses->set_sid(session_manager::get_next_sessionid());

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

    SSL_CTX_set_ciphersuites(ctx, 
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256");

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TICKET | SSL_OP_NO_COMPRESSION);

    SSL_CTX_set_tlsext_status_cb(ctx, ocsp_callback); // OCSP Stapling

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

// 请求头安全检查
bool httpsession_manager::check_headers(const httpprotocol* req)
{
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024; // 1MB
    if (req->get_header("Content-Length").size() > MAX_BODY_SIZE) return false;

    // 检查Host头
    if (!req->has_header("Host")) return false;

    // 限制头数量
    constexpr size_t MAX_HEADERS = 64;
    if (req->header_count() > MAX_HEADERS) return false;

    // 防御请求走私攻击
    if (req->has_header("Transfer-Encoding") && 
        req->has_header("Content-Length")) return false;

    // 检查头字段合法性
    static const std::set<std::string> ALLOWED_HEADERS = {
        "Host", "User-Agent", "Accept", "Content-Length", 
        "Content-Type", "Connection"
    };

    for (const auto& [k, v] : req->headers())
    {
        // 检查字段名合法性
        if (k.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos)
            return false;
        
        // 白名单检查
        if (!ALLOWED_HEADERS.contains(k)) return false;
    }
    return true;
}


void httpsession_manager::ocsp_callback(SSL* ssl, void* arg)
{

}

} // namespace bee