#include "httpsession.h"
#include "httpprotocol.h"
#include "httpsession_manager.h"
#include "systemtime.h"
#include <openssl/err.h>
#include <print>

namespace bee
{

httpsession::httpsession(httpsession_manager* manager)
    : session(manager) {}

httpsession::~httpsession()
{
    if (_ssl)
    {
        SSL_shutdown(_ssl);
        SSL_free(_ssl);
    }
}

void httpsession::open()
{
    session::open();
    if (_ssl)
    {
        SSL_set_fd(_ssl, _sockfd);
        int ret = SSL_accept(_ssl);
        if (ret <= 0)
        {
            int err = SSL_get_error(_ssl, ret);
            ERR_print_errors_fp(stderr);
            std::println("SSL handshake failed for session %lu with error: %d", _sid, err);
            close();
            return;
        }
        std::println("SSL handshake successful for session %lu", _sid);
    }
    else
    {
        std::println("Non-SSL session opened for session %lu", _sid);
    }
}

void httpsession::close()
{
    if (_ssl)
    {
        SSL_shutdown(_ssl);
    }
    session::close();
}

void httpsession::update_last_active()
{
    _last_active = systemtime::get_millseconds();
}

bool httpsession::is_timeout(TIMETYPE timeout) const
{
    return (systemtime::get_millseconds() - _last_active) > timeout;
}

void httpsession::on_recv(size_t len)
{
    update_last_active();
    if (_ssl)
    {
        char buffer[4096];
        int ret = SSL_read(_ssl, buffer, sizeof(buffer));
        if (ret > 0)
        {
            _readbuf.append(buffer, ret);
            session::on_recv(ret);
        }
        else
        {
            int err = SSL_get_error(_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                return;
            }
            ERR_print_errors_fp(stderr);
            std::println("SSL_read failed for session %lu with error: %d", _sid, err);
            close();
        }
    }
    else
    {
        session::on_recv(len);
    }
}

void httpsession::on_send(size_t len)
{
    update_last_active();
    if (_ssl)
    {
        const char* data = _writebuf.data();
        size_t remaining = _writebuf.size();
        while (remaining > 0)
        {
            int ret = SSL_write(_ssl, data, remaining);
            if (ret > 0)
            {
                data += ret;
                remaining -= ret;
            }
            else
            {
                int err = SSL_get_error(_ssl, ret);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                {
                    return;
                }
                ERR_print_errors_fp(stderr);
                std::println("SSL_write failed for session %lu with error: %d", _sid, err);
                close();
                return;
            }
        }
        _writebuf.clear();
    }
    else
    {
        session::on_send(len);
    }
}

void httpsession::handle_request(httpprotocol* prot)
{
    auto req = dynamic_cast<httprequest*>(prot);
    if (!req) return;

    req->set_callback([this](httpresponse* response)
    {
        octetsstream os;
        response->pack(os);
        _writeos.data().append(os.data(), os.size());
        permit_send();
    });
    req->run();
    delete req;
}

} // namespace bee