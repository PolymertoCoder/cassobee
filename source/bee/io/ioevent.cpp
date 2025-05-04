#include <cerrno>
#include <openssl/bio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>
#include <openssl/err.h>

#include "ioevent.h"
#include "glog.h"
#include "address.h"
#include "event.h"
#include "common.h"
#include "reactor.h"
#include "session.h"

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
            local_log("stdio_event handle_event read %d bytes from stdin.", len);
        }
        else if(len == 0)
        {
            local_log("stdio_event handle_event stdin closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                local_log("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
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
            local_log("stdio_event handle_event write %d bytes to stdout.", len);
        }
        else if(len == 0)
        {
            local_log("stdio_event handle_event stdout closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                local_log("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
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
        local_log("fd %d listen addr:%s.", listenfd, manager->get_addr()->to_string().data());
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

    streamio_event* evt = new streamio_event(clientfd, _ses->dup());
    evt->set_events(EVENT_RECV | EVENT_SEND);
    _base->add_event(evt);
    local_log("accept clientid=%d.", clientfd);
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

    // 所有权转移给新的streamio_event
    streamio_event* evt = new streamio_event(_fd, _ses);
    evt->set_events(EVENT_RECV | EVENT_SEND | EVENT_HUP);
    evt->set_status(EVENT_STATUS_ADD);
    _base->add_event(evt);
    local_log("activeio_event handle_event run fd=%d.", _fd);
    _fd = -1;
    _ses = nullptr;
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
    //local_log("streamio_event constructor run");
}

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_recv();
        // local_log("streamio_event handle_event EVENT_RECV fd=%d", _fd);
    }
    if(active_events & EVENT_SEND)
    {
        handle_send();
        // local_log("streamio_event handle_event EVENT_SEND fd=%d", _fd);
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
        int len = recv(_fd, recv_ptr, free_space, 0);
        if(len > 0)
        {
            rbuffer.fast_resize(len);
            total_recv += len;
            local_log("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
                _fd, len, rbuffer.size(), rbuffer.free_space());
        }
        else if(len == 0)
        {
            local_log("recv[fd=%d]: connection closed by peer, buffer size=%zu",
                _fd, rbuffer.size());
            close_socket();
            return -1;
        }
        else // len < 0
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                // 非阻塞模式下的正常情况，不需要额外处理
                // local_log("recv[fd=%d]: temporary error (errno=%d: %s), buffer size=%zu, free space=%zu",
                //     _fd, errno, strerror(errno), rbuffer.size(), rbuffer.free_space());
                break;
            }
            else
            {
                perror("recv");
                local_log("recv[fd=%d]: error (errno=%d: %s), closing socket, buffer size=%zu",
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
        if(wbuffer->size() == 0)
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
            //local_log("send data:%s", std::string(wbuffer.peek(0), len).data());
            total_send += len;
            _ses->_write_offset += len;
        }
        else if(len == 0)
        {
            if(_ses->_write_offset < wbuffer->size())
            {
                local_log("Send returned 0: Connection might be closed by the peer.\n");
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

} // namespace bee