#include <asm-generic/socket.h>
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
    _ses->close();
    close(_fd);
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
        TRACELOG("addr:%s.", manager->get_addr()->to_string().data());
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

    int family = _ses->get_manager()->family();
    if(family == AF_INET)
    {
        struct sockaddr_in sock_client;
        socklen_t len = sizeof(sock_client);
        int clientfd = accept(_fd, (sockaddr*)&sock_client, &len);
        if(clientfd == -1)
        {
            if(errno != EAGAIN || errno != EINTR)
            {
                TRACELOG("accept: %s\n", strerror(errno));
                return false;
            }
        }

        int ret = set_nonblocking(clientfd);
        if(ret < 0) return false;


        auto evt = new streamio_event(clientfd, _ses->dup());
        evt->set_events(EVENT_RECV);
        _base->add_event(evt);
        TRACELOG("accept clientid=%d.", clientfd);
    }
    else if(family  == AF_INET6)
    {
        struct sockaddr_in6 sock_client;
        socklen_t len = sizeof(sock_client);
        int clientfd = accept(_fd, (sockaddr*)&sock_client, &len);
        if(clientfd == -1)
        {
            if(errno != EAGAIN || errno != EINTR)
            {
                TRACELOG("accept: %s", strerror(errno));
                return false;
            }
        }

        if(set_nonblocking(clientfd) < 0) return false;

        auto evt = new streamio_event(clientfd, _ses->dup());
        evt->set_events(EVENT_RECV);
        _base->add_event(evt);
        TRACELOG("accept clientid=%d.", clientfd);
    }
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
        perror("connect");
        //_base->del_event(this);
        return false;
    }
    int optval = 0; socklen_t optlen = sizeof(optval);
    if(getsockopt(_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
    {
        perror("getsockopt");
        close_socket();
        return false;
    }

    //_base->del_event(this);
    auto evt = new streamio_event(_fd, _ses->dup());
    evt->set_events(EVENT_SEND);
    evt->set_status(EVENT_STATUS_ADD);
    _base->add_event(evt);
    printf("activeio_event handle_event run fd=%d.\n", _fd);
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
    printf("streamio_event constructor run\n");
}

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_recv();
        _ses->permit_send();
        printf("streamio_event handle_event EVENT_RECV fd=%d\n", _fd);
    }
    else if(active_events & EVENT_SEND)
    {
        handle_send();
        _ses->permit_recv();
        printf("streamio_event handle_event EVENT_SEND fd=%d\n", _fd);
    }
    return true;
}

int streamio_event::handle_recv()
{
    if(_base == nullptr) return -1;

#if 0 //ET
    int idx = 0, len = 0;
    while(idx < READ_BUFFER_SIZE)
    {
        len = recv(_fd, _ses->_readbuf.data() + idx, READ_BUFFER_SIZE-idx, 0);
        if(len > 0) {
            idx += len;
            local_log("recv[%d]:%s", len, _ses->_readbuf.data());
        }
        else if(len == 0) {
            close(_fd);
        }
        else {
            close(_fd);
        }
    }

    if(idx == READ_BUFFER_SIZE && len != -1) {
        _base->add_event(this, EVENT_RECV); 
    } else if(len == 0) {
        //base->add_event(this, EVENT_RECV); 
    } else {
        _base->add_event(this, EVENT_SEND);
    }

#else //LT
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0) return 0;

    int len = recv(_fd, rbuffer.end(), free_space, 0);
    if(len > 0)
    {
        rbuffer.fast_resize(len);
        // 处理业务
        TRACELOG("recv data:%s", std::string(rbuffer.end()-len, len).data());
        _ses->on_recv(len);
    }
    else if(len == 0)
    {
        TRACELOG("sockfd %d disconnected", _fd);
        //close(_fd);
    }
    else
    {
        TRACELOG("recv[fd=%d] len=%d error[%d]:%s", _fd, len, errno, strerror(errno));
        close(_fd);
    }
    if(rbuffer.free_space() == 0)
    {
        _ses->forbid_recv();
    }
#endif
    return len;
}

int streamio_event::handle_send()
{
    if(_base == nullptr) return -1;

    octets& wbuffer = _ses->wbuffer();
    int len = send(_fd, wbuffer.begin(), wbuffer.size(), 0);
    if(len > 0)
    {
        printf("send data:%s\n", std::string(wbuffer.peek(0), len).data());
        wbuffer.erase(0, len);
        _ses->on_send(len);
        _ses->permit_recv();
    }
    else if(len == 0)
    {
        _ses->permit_send();
    }
    else
    {
        perror("send");
    }
    if(wbuffer.size() == 0)
    {
        _ses->forbid_send();
    }
    return len;
}