#include "sslio_event.h"
#include "address.h"
#include "glog.h"
#include "ioevent.h"
#include "reactor.h"
#include "systemtime.h"
#include <openssl/ssl.h>

namespace bee
{

ssl_passiveio_event::ssl_passiveio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    set_events(EVENT_ACCEPT);
    int socktype = _ses->get_manager()->socktype();
    
    if(socktype == SOCK_STREAM)
    {
        int family = manager->family();
        int listenfd = socket(family, socktype, 0);
        int opt = 1;
        if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            close(listenfd);
            exit(EXIT_FAILURE);
        }    
        set_nonblocking(listenfd);

        struct sockaddr* addr = manager->get_addr()->addr();
        if(bind(listenfd, addr, sizeof(*addr)) < 0)
        {
            perror("bind");
            close(listenfd);
            return;
        }
        if(listen(listenfd, 20) < 0)
        {
            perror("listen");
            close(listenfd);
            return;
        }
        _fd = listenfd;
        local_log("fd %d listen addr:%s.", listenfd, manager->get_addr()->to_string().data());
    }
}

bool ssl_passiveio_event::handle_event(int active_events)
{
    if(_base == nullptr) return false;
    if(active_events != EVENT_ACCEPT) return false;

    struct sockaddr_storage sock_client;
    socklen_t len = sizeof(sock_client);
    int clientfd = accept(_fd, (struct sockaddr*)&sock_client, &len);
    if(clientfd < 0)
    {
        if (errno != EAGAIN && errno != EINTR)
        {
            local_log("accept: %s", strerror(errno));
            return false;
        }
        return true;
    }
    if(set_nonblocking(clientfd) < 0)
    {
        close(clientfd);
        return false;
    }

    sslio_event* evt = new sslio_event(clientfd, _ses->get_manager()->get_ssl_ctx(), true/*server*/, _ses->dup());
    evt->set_events(EVENT_RECV | EVENT_SEND);
    _base->add_event(evt);
    local_log("accept ssl clientid=%d.", clientfd);
    return true;
}

ssl_activeio_event::ssl_activeio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    set_events(EVENT_SEND);
    _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_fd < 0)
    {
        perror("create socket");
        return;
    }
    set_nonblocking(_fd);
}

bool ssl_activeio_event::handle_event(int active_events)
{
    if(_base == nullptr) return false;

    struct sockaddr* addr = _ses->get_manager()->get_addr()->addr();
    if(connect(_fd, addr, sizeof(*addr)) < 0)
    {
        if(errno != EINPROGRESS)
        {
            perror("connect");
            close_socket();
            return false;
        }
    }

    int optval = 0; socklen_t optlen = sizeof(optval);
    if(getsockopt(_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
    {
        if(optval != 0)
        {
            errno = optval;
            perror("connect");
        }
        else
        {
            perror("getsockopt");
        }
        close_socket();
        return false;
    }

    // 所有权转移给新的sslio_event
    sslio_event* evt = new sslio_event(_fd, _ses->get_manager()->get_ssl_ctx(), false/*client*/, _ses);
    evt->set_events(EVENT_RECV | EVENT_SEND | EVENT_HUP);
    evt->set_status(EVENT_STATUS_ADD);
    _base->add_event(evt);
    local_log("ssl_activeio_event handle_event run fd=%d.", _fd);
    _fd = -1;
    _ses = nullptr;
    return true;
}

sslio_event::sslio_event(int fd, SSL_CTX* ssl_ctx, bool is_server, session* ses)
    : netio_event(ses), _ssl_ctx(ssl_ctx), _is_server(is_server)
{
    _fd = fd;
    _ssl = SSL_new(ssl_ctx);
    SSL_set_fd(_ssl, _fd);
    _handshake_starttime = systemtime::get_time();

    if(_is_server) {
        SSL_set_accept_state(_ssl);
    } else {
        SSL_set_connect_state(_ssl);
    }
}

sslio_event::~sslio_event()
{
    cleanup_ssl();
}

bool sslio_event::handle_event(int active_events)
{
    if(!_handshake_done)
    {
        return handle_handshake();
    }
    if(active_events & EVENT_RECV)
    {
        handle_recv();
        local_log("sslio_event handle_event EVENT_RECV fd=%d", _fd);
    }
    if(active_events & EVENT_SEND)
    {
        handle_send();
        local_log("sslio_event handle_event EVENT_SEND fd=%d", _fd);
    }
    return true;
}

bool sslio_event::handle_handshake()
{
    // 检查握手超时（30s）
    TIMETYPE curtime = systemtime::get_time();
    if(curtime - _handshake_starttime > 30)
    {
        local_log("sslio_event handle_handshake timeout fd=%d", _fd);
        cleanup_ssl();
        close_socket();
        return false;
    }

    int ret = SSL_do_handshake(_ssl);
    if(ret == 1)
    {
        _handshake_done = true;
        local_log("sslio_event handle_handshake success fd=%d", _fd);
        return true;
    }

    int err = SSL_get_error(_ssl, ret);
    switch(err)
    {
        case SSL_ERROR_WANT_READ:
        {
            _ses->permit_recv();
            local_log("sslio_event handle_handshake SSL_ERROR_WANT_READ fd=%d", _fd);
        } break;
        case SSL_ERROR_WANT_WRITE:
        {
            _ses->permit_send();
            local_log("sslio_event handle_handshake SSL_ERROR_WANT_WRITE fd=%d", _fd);
        } break;
        default:
        {
            local_log("sslio_event handle_handshake error fd=%d", _fd);
            cleanup_ssl();
            close_socket();
            return false;
        } break;
    }
    return true;
}

int sslio_event::handle_recv()
{
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0)
    {
        _ses->on_recv(rbuffer.size());
        local_log("recv[fd=%d]: session %lu buffer is fulled, no space to receive data", _fd, _ses->get_sid());
    }
    
    int total_recv = 0;
    while(true)
    {
        size_t free_space = rbuffer.free_space();
        if(free_space == 0)
        {
            local_log("recv[fd=%d]: session %lu buffer is fulled on receiving", _fd, _ses->get_sid());
            break;
        }

        char* recv_ptr = rbuffer.end();
        int len = SSL_read(_ssl, recv_ptr, free_space);
        if(len > 0)
        {
            rbuffer.fast_resize(len);
            total_recv += len;
            local_log("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
                _fd, len, rbuffer.size(), rbuffer.free_space());
        }
        else
        {
            int err = SSL_get_error(_ssl, len);
            if(err == SSL_ERROR_WANT_READ) break;
            if(err == SSL_ERROR_WANT_WRITE)
            {
                _ses->permit_send();
                break;
            }
            cleanup_ssl();
            close_socket();
            local_log("sslio_event handle_recv error fd=%d", _fd);
            return -1;
        }
    }
    
    _ses->on_recv(total_recv);
    return total_recv;
}

// 优化SSL写操作（减少内存拷贝）
int sslio_event::handle_send()
{
    octets* wbuffer = nullptr;
    {
        bee::rwlock::wrscoped sesl(_ses->_locker);
        wbuffer = &_ses->wbuffer();
        if(wbuffer->size() == 0)
        {
            _ses->forbid_send();
            return 0;
        }
    }

    int total_send = 0;
    while(true)
    {
        int len = SSL_write(_ssl, wbuffer->data() + _ses->_write_offset, wbuffer->size() - _ses->_write_offset);
        if(len > 0)
        {
            //local_log("SSL_write data:%s", std::string(wbuffer.peek(0), len).data());
            total_send += len;
            _ses->_write_offset += len;
        }
        else if(len == 0)
        {
            if(_ses->_write_offset < wbuffer->size())
            {
                local_log("SSL_write[fd=%d]: session %lu buffer is fulled, no space to send data", _fd, _ses->get_sid());
                bee::rwlock::wrscoped sesl(_ses->_locker);
                _ses->permit_send(); // 写缓冲区的数据还有剩余，下次唤醒时再尝试
            }
            break;
        }
        else // len < 0
        {
            int err = SSL_get_error(_ssl, len);
            if(err == SSL_ERROR_WANT_WRITE) break;
            if(err == SSL_ERROR_WANT_READ)
            {
                _ses->permit_recv();
                break;
            }
            cleanup_ssl();
            close_socket();
            local_log("sslio_event handle_send error fd=%d", _fd);
            return -1;
        }

        if(_ses->_write_offset == wbuffer->size())
        {
            _ses->clear_wbuffer();
            bee::rwlock::wrscoped sesl(_ses->_locker);
            wbuffer = &_ses->wbuffer();
            if(wbuffer->size() == _ses->_write_offset)
            {
                _ses->forbid_send();
                break;
            }
        }
    }

    _ses->on_send(total_send);
    return total_send;
}

void sslio_event::cleanup_ssl()
{
    if(_ssl)
    {
        SSL_shutdown(_ssl);
        SSL_free(_ssl);
        _ssl = nullptr;
    }
}

} // namespace bee