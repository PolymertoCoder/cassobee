#include <cerrno>
#include <print>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>

#include "ioevent.h"
#include "address.h"
#include "event.h"
#include "log.h"
#include "common.h"
#include "reactor.h"
#include "session.h"

netio_event::netio_event(session* ses) : _ses(ses)
{
    _ses->set_event(this);
}

netio_event::~netio_event()
{
    close_socket();
    delete _ses;
}

void netio_event::close_socket()
{
    if(_fd >= 0)
    {
        ::close(_fd);
        _fd = -1;
        _ses->close();
    }
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

    auto evt = new streamio_event(clientfd, _ses->dup());
    evt->set_events(EVENT_RECV | EVENT_SEND);
    _base->add_event(evt);
    std::println("accept clientid=%d.", clientfd);
    return 0;
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

    //_base->del_event(this);
    auto evt = new streamio_event(_fd, _ses->dup());
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
    ses->open();
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
    else if(active_events & EVENT_SEND)
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
    if(_base == nullptr) return -1;

    bee::rwlock::wrscoped sesl(_ses->_locker);
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0)
    {
        std::println("recv[fd=%d]: buffer is full, no space to receive data", _fd);
        return 0;
    }

    char* recv_ptr = rbuffer.end();
    int len = recv(_fd, recv_ptr, free_space, 0);
    if(len > 0)
    {
        rbuffer.fast_resize(len);
        std::println("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
              _fd, len, rbuffer.size(), rbuffer.free_space());
        _ses->on_recv(len);
    }
    else if(len == 0)
    {
        std::println("recv[fd=%d]: connection closed by peer, buffer size=%zu",
              _fd, rbuffer.size());
        close_socket();
    }
    else // len < 0
    {
        if(errno == EAGAIN || errno == EINTR)
        {
            // 非阻塞模式下的正常情况，不需要额外处理
            std::println("recv[fd=%d]: temporary error (errno=%d: %s), buffer size=%zu, free space=%zu",
                  _fd, errno, strerror(errno), rbuffer.size(), rbuffer.free_space());
        }
        else
        {
            perror("recv");
            std::println("recv[fd=%d]: error (errno=%d: %s), closing socket, buffer size=%zu",
                  _fd, errno, strerror(errno), rbuffer.size());
            close_socket();
        }
        std::println("recv[fd=%d] len=%d error[%d]:%s", _fd, len, errno, strerror(errno));
    }
    if(rbuffer.free_space() == 0)
    {
        _ses->forbid_recv();
    }
    return len;
}

int streamio_event::handle_send()
{
    if(_base == nullptr) return -1;

    int total_send = 0;
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