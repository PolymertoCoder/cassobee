#include "httpsession.h"
#include "httpprotocol.h"
#include "httpsession_manager.h"
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
        if (SSL_accept(_ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            close();
        }
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

void httpsession::on_recv(size_t len)
{
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
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            {
                ERR_print_errors_fp(stderr);
                close();
            }
        }
    }
    else
    {
        session::on_recv(len);
    }
}

void httpsession::on_send(size_t len)
{
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
                if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
                {
                    ERR_print_errors_fp(stderr);
                    close();
                    return;
                }
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
    prot->set_callback([this](httpprotocol* response)
    {
        octetsstream os;
        response->encode(os);
        _writeos.data().append(os.data(), os.size());
        permit_send();
    });
    prot->run();
    delete prot;
}

} // namespace bee