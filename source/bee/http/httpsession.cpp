#include "httpsession.h"
#include "address.h"
#include "glog.h"
#include "httpprotocol.h"
#include "threadpool.h"
#include "httpsession_manager.h"
#include "session.h"
#include <openssl/err.h>

namespace bee
{

httpsession::httpsession(httpsession_manager* manager)
    : session(manager)
{
    if(manager->ssl_enabled())
    {
        _ssl = SSL_new(((httpsession_manager*)_manager)->get_ssl_ctx());
    }
}

httpsession::~httpsession()
{
    if(_ssl)
    {
        SSL_shutdown(_ssl);
        SSL_free(_ssl);
        _ssl = nullptr;
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
    if(_ssl)
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
    if(_ssl)
    {
        SSL_shutdown(_ssl);
    }
    session::set_close();
}

void httpsession::on_recv(size_t len)
{
    activate();

    local_log("httpsession::on_recv len=%zu, _readbuf size:%zu _reados size:%zu.", len, _readbuf.size(), _reados.size());
    set_state(SESSION_STATE_RECVING);

    size_t append_length = std::min(_readbuf.size(), _reados.data().free_space());
    _reados.data().append(_readbuf, append_length);
    _readbuf.erase(0, append_length);
    local_log("on_recv append size:%zu, _readbuf size:%zu _reados size:%zu.", append_length, _readbuf.size(), _reados.size());

    while(httpprotocol* prot = httpprotocol::decode(_reados, this))
    {
    #ifdef _REENTRANT
        threadpool::get_instance()->add_task(prot->thread_group_idx(), [prot]()
        {
            prot->run();
            delete prot;
        });
    #else
        prot->run();
    #endif
    }
    _reados.try_shrink();
}

void httpsession::on_send(size_t len)
{
    activate();

    if(_ssl)
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