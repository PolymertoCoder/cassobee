#include "httpsession.h"
#include "address.h"
#include "glog.h"
#include "httpprotocol.h"
#include "httpsession_manager.h"
#include "session.h"
#include <openssl/err.h>

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

httpsession* httpsession::dup()
{
    auto ses = new httpsession(*this);
    ses->_sid = 0;
    ses->_sockfd = 0;
    ses->_state = SESSION_STATE_NONE;
    ses->_peer = _peer->dup();

    ses->_event = nullptr;
    ses->_readbuf.clear();
    ses->_writebuf.clear();
    ses->_reados.clear();
    ses->_writeos.clear();

    if(_ssl)
    {
        ses->_ssl = SSL_new(((httpsession_manager*)_manager)->get_ssl_ctx());
    }
    return ses;
}

void httpsession::set_open()
{
    session::set_open();
    if (_ssl)
    {
        SSL_set_fd(_ssl, _sockfd);
        SSL_set_mode(_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        int ret = SSL_accept(_ssl);
        if (ret <= 0)
        {
            int err = SSL_get_error(_ssl, ret);
            ERR_print_errors_fp(stderr);
            local_log("SSL handshake failed for session %lu with error: %d", _sid, err);
            close();
            return;
        }
        local_log("SSL handshake successful for session %lu", _sid);
    }
    else
    {
        local_log("Non-SSL session opened for session %lu", _sid);
    }
}

void httpsession::set_close()
{
    if (_ssl)
    {
        SSL_shutdown(_ssl);
    }
    session::set_close();
}

void httpsession::on_recv(size_t len)
{
    activate();

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
            local_log("SSL_read failed for session %lu with error: %d", _sid, err);
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
    activate();

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
                local_log("SSL_write failed for session %lu with error: %d", _sid, err);
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

} // namespace bee