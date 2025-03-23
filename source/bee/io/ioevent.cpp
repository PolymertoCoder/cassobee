#include <cerrno>
#include <openssl/bio.h>
#include <print>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>
#include <openssl/err.h>

#include "ioevent.h"
#include "address.h"
#include "event.h"
#include "common.h"
#include "reactor.h"
#include "session.h"
#include "httpsession.h"
#include "httpsession_manager.h"

namespace bee
{

stdio_event::stdio_event(int fd, size_t buffer_size)
{
    _fd = fd;
    _buffer.reserve(buffer_size);
    set_nonblocking(_fd);
}

size_t stdio_event::on_read()
{
    size_t total_read = 0;
    while(true)
    {
        size_t free_size = _buffer.free_space();
        if(free_size == 0) break;

        char* recv_ptr = _buffer.end();
        int len = read(_fd, recv_ptr, free_size);
        if(len > 0)
        {
            _buffer.fast_resize(len);
            total_read += len;
            std::println("stdio_event handle_event read %d bytes from stdin.", len);
        }
        else if(len == 0)
        {
            std::println("stdio_event handle_event stdin closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                std::println("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
                return true;
            }
            else
            {
                perror("read");
                return false;
            }
        }
    }
    return total_read;
}

size_t stdio_event::on_write()
{
    size_t total_write = 0;
    while(true)
    {
        size_t free_size = _buffer.free_space();
        if(free_size == 0) break;

        char* recv_ptr = _buffer.end();
        int len = write(_fd, recv_ptr, free_size);
        if(len > 0)
        {
            total_write += len;
            std::println("stdio_event handle_event write %d bytes to stdout.", len);
        }
        else if(len == 0)
        {
            std::println("stdio_event handle_event stdout closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                std::println("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
                return true;
            }
            else
            {
                perror("write");
                return false;
            }
        }
    }
    return total_write;
}

stdin_event::stdin_event(size_t buffer_size)
    : stdio_event(fileno(stdin), buffer_size)
{
    set_events(EVENT_RECV);
}

bool stdin_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        size_t total_read = stdio_event::on_read();
        return handle_read(total_read);
    }
    return true;
}

stdout_event::stdout_event(size_t buffer_size)
    : stdio_event(fileno(stdout), buffer_size)
{
    set_events(EVENT_SEND);
}

bool stdout_event::handle_event(int active_events)
{
    if(active_events & EVENT_SEND)
    {
        size_t total_write = stdio_event::on_write();
        return handle_write(total_write);
    }
    return true;
}

stderr_event::stderr_event(size_t buffer_size)
    : stdio_event(fileno(stderr), buffer_size)
{
    set_events(EVENT_RECV);
}

bool stderr_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        size_t total_read = stdio_event::on_read();
        return handle_error(total_read);
    }
    return true;
}

netio_event::netio_event(session* ses) : _ses(ses)
{
    _ses->set_event(this);
}

netio_event::~netio_event()
{
    if(_fd >= 0)
    {
        ::close(_fd);
        _fd = -1;
    }
    if(_ses)
    {
        _ses->set_close();
        delete _ses;
        _ses = nullptr;
    }
}

void netio_event::close_socket()
{
    _base->del_event(this);
}

passiveio_event::passiveio_event(session_manager* manager)
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
        std::println("fd %d listen addr:%s.", listenfd, manager->get_addr()->to_string().data());
    }
    else if(socktype == SOCK_DGRAM)
    {
        // TODO
    }
}

bool passiveio_event::handle_event(int active_events)
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
            std::println("accept: %s", strerror(errno));
            return false;
        }
        return true;
    }
    if(set_nonblocking(clientfd) < 0)
    {
        close(clientfd);
        return false;
    }

    streamio_event* evt = nullptr;

    if(_ses->get_manager()->is_httpsession_manager())
    {
        evt = new sslio_event(clientfd, ((bee::httpsession*)_ses)->dup());
    }
    else
    {
        evt = new streamio_event(clientfd, _ses->dup());
    }

    evt->set_events(EVENT_RECV | EVENT_SEND);
    _base->add_event(evt);
    std::println("accept clientid=%d.", clientfd);
    return true;
}

activeio_event::activeio_event(session_manager* manager)
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

bool activeio_event::handle_event(int active_events)
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

    streamio_event* evt = nullptr;

    if(_ses->get_manager()->is_httpsession_manager())
    {
        evt = new sslio_event(_fd, (bee::httpsession*)_ses);
    }
    else
    {
        evt = new streamio_event(_fd, _ses);
    }

    // 所有权转移给新的streamio_event
    _fd = -1;
    _ses = nullptr; 
    evt->set_events(EVENT_RECV | EVENT_SEND | EVENT_HUP);
    evt->set_status(EVENT_STATUS_ADD);
    _base->add_event(evt);
    std::println("activeio_event handle_event run fd=%d.", _fd);
    return true;
}

streamio_event::streamio_event(int fd, session* ses)
    : netio_event(ses)
{
    _fd = fd;
    address* addr = ses->get_peer();
    if(getsockname(_fd, addr->addr(), &addr->len()) < 0)
    {
        perror("getsockname");
        close(_fd);
        exit(-1);
    }
    int flag = 1;
    if(setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int)) < 0)
    {
        perror("setsockopt");
        close(_fd);
        exit(-1);
    }
    if(setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
    {
        perror("setsockopt");
        close(_fd);
        exit(-1);
    }
    ses->set_open();
    //std::println("streamio_event constructor run");
}

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_recv();
        // _ses->permit_recv();
        // _ses->permit_send();
        std::println("streamio_event handle_event EVENT_RECV fd=%d", _fd);
    }
    if(active_events & EVENT_SEND)
    {
        handle_send();
        // _ses->permit_recv();
        // _ses->permit_send();
        std::println("streamio_event handle_event EVENT_SEND fd=%d", _fd);
    }
    return true;
}

int streamio_event::handle_recv()
{
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0)
    {
        _ses->on_recv(rbuffer.size());
        std::println("recv[fd=%d]: session %llu buffer is fulled, no space to receive data", _ses->get_sid(), _fd);
    }
    
    int total_recv = 0;
    while(true)
    {
        size_t free_space = rbuffer.free_space();
        if(free_space == 0)
        {
            std::println("recv[fd=%d]: session %llu buffer is fulled on receiving", _ses->get_sid(), _fd);
            break;
        }

        char* recv_ptr = rbuffer.end();
        int len = recv(_fd, recv_ptr, free_space, 0);
        if(len > 0)
        {
            rbuffer.fast_resize(len);
            total_recv += len;
            std::println("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
                _fd, len, rbuffer.size(), rbuffer.free_space());
        }
        else if(len == 0)
        {
            std::println("recv[fd=%d]: connection closed by peer, buffer size=%zu",
                _fd, rbuffer.size());
            close_socket();
            return -1;
        }
        else // len < 0
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                // 非阻塞模式下的正常情况，不需要额外处理
                std::println("recv[fd=%d]: temporary error (errno=%d: %s), buffer size=%zu, free space=%zu",
                    _fd, errno, strerror(errno), rbuffer.size(), rbuffer.free_space());
                break;
            }
            else
            {
                perror("recv");
                std::println("recv[fd=%d]: error (errno=%d: %s), closing socket, buffer size=%zu",
                    _fd, errno, strerror(errno), rbuffer.size());
                close_socket();
                return -1;
            }
        }
    }
    
    _ses->on_recv(total_recv);
    return total_recv;
}

int streamio_event::handle_send()
{
    octets* wbuffer = nullptr;
    {
        bee::rwlock::wrscoped sesl(_ses->_locker);
        wbuffer = &_ses->wbuffer();
        if(wbuffer->size() == _ses->_write_offset)
        {
            _ses->forbid_send();
            return 0;
        }
    }

    int total_send = 0;
    while(true)
    {
        int len = send(_fd, wbuffer->data() + _ses->_write_offset, wbuffer->size() - _ses->_write_offset, 0);
        if(len > 0)
        {
            //std::println("send data:%s", std::string(wbuffer.peek(0), len).data());
            total_send += len;
            _ses->_write_offset += len;
        }
        else if(len == 0)
        {
            if(_ses->_write_offset < wbuffer->size())
            {
                std::println("Send returned 0: Connection might be closed by the peer.\n");
                bee::rwlock::wrscoped sesl(_ses->_locker);
                _ses->permit_send(); // 写缓冲区的数据还有剩余，下次唤醒时再尝试
            }
            break;
        }
        else // len < 0
        {
            if(errno != EAGAIN && errno != EINTR)
            {
                perror("send");
                // close_socket();
                break;
            }
        }
        if(_ses->_write_offset == wbuffer->size())
        {
            _ses->clear_wbuffer();
            bee::rwlock::wrscoped sesl(_ses->_locker);
            wbuffer = &_ses->wbuffer();
            if(wbuffer->size() == 0)
            {
                _ses->forbid_send();
                break;
            }
        }
    }

    _ses->on_send(total_send);
    return total_send;
}

sslio_event::sslio_event(int fd, httpsession* ses)
    : streamio_event(fd, ses), _ssl(ses->get_ssl()) {}

bool sslio_event::do_handshake()
{
    int ret = SSL_do_handshake(_ssl);
    if (ret == 1) return true;

    int err = SSL_get_error(_ssl, ret);
    switch(err)
    {
        case SSL_ERROR_WANT_READ:
        {
            _ses->permit_recv();
            return true;
        }
        case SSL_ERROR_WANT_WRITE:
        {
            _ses->permit_send();
            return true;
        }
        default:
        {
            char errbuf[256];
            ERR_error_string_n(ERR_get_error(), errbuf, sizeof(errbuf));
            std::println("SSL handshake failed: %s", errbuf);
            return false;
        }
    }
}

int sslio_event::handle_recv()
{
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0)
    {
        _ses->on_recv(rbuffer.size());
        std::println("recv[fd=%d]: session %llu buffer is fulled, no space to receive data", _ses->get_sid(), _fd);
    }
    
    int total_recv = 0;
    while(true)
    {
        size_t free_space = rbuffer.free_space();
        if(free_space == 0)
        {
            std::println("recv[fd=%d]: session %llu buffer is fulled on receiving", _ses->get_sid(), _fd);
            break;
        }

        char* recv_ptr = rbuffer.end();
        int len = SSL_read(_ssl, recv_ptr, free_space);
        if(len > 0)
        {
            rbuffer.fast_resize(len);
            total_recv += len;
            std::println("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
                _fd, len, rbuffer.size(), rbuffer.free_space());
        }
        else
        {
            int err = SSL_get_error(_ssl, len);
            if (err == SSL_ERROR_WANT_READ) break;
            if (err == SSL_ERROR_WANT_WRITE)
            {
                _ses->permit_send();
                break;
            }
            close_socket();
            return -1;
        }
    }
    
    _ses->on_recv(total_recv);
    return total_recv;
}

// #include "traffic_shaper.h"

// 优化SSL写操作（减少内存拷贝）
int sslio_event::handle_send()
{
    octets* wbuffer = nullptr;
    {
        bee::rwlock::wrscoped sesl(_ses->_locker);
        wbuffer = &_ses->wbuffer();
        if(wbuffer->size() == _ses->_write_offset)
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
            //std::println("send data:%s", std::string(wbuffer.peek(0), len).data());
            total_send += len;
            _ses->_write_offset += len;
        }
        else // len < 0
        {
            int err = SSL_get_error(_ssl, len);
            if(err == SSL_ERROR_WANT_WRITE) break;
            close_socket();
            return -1;
        }

        if(_ses->_write_offset == wbuffer->size())
        {
            _ses->clear_wbuffer();
            bee::rwlock::wrscoped sesl(_ses->_locker);
            wbuffer = &_ses->wbuffer();
            if(wbuffer->size() == 0)
            {
                _ses->forbid_send();
                break;
            }
        }
    }

    _ses->on_send(total_send);
    return total_send;
}

} // namespace bee