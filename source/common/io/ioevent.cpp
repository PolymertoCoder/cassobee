#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>

#include "ioevent.h"
#include "address.h"
#include "log.h"
#include "common.h"
#include "reactor.h"
#include "session.h"

passiveio_event::passiveio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    int socktype = _ses->get_manager()->socktype();
    
    if(socktype == SOCK_STREAM)
    {
        int family = manager->family();
        int listenfd = socket(family, socktype, 0);
        set_nonblocking(listenfd);

        struct sockaddr* addr = manager->get_addr()->addr();
        TRACELOG("addr:%s.", manager->get_addr()->to_string().data());
        if(bind(listenfd, addr, sizeof(*addr)) < 0)
        {
            perror("bind");
            close(listenfd);
            exit(-1);
        }
        if(listen(listenfd, 20) < 0)
        {
            perror("listen");
            close(listenfd);
            exit(-1);
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

        _base->add_event(new streamio_event(clientfd, _ses->dup()), EVENT_RECV);
        TRACELOG("accept clientid=%d.\n", clientfd);
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
                TRACELOG("accept: %s\n", strerror(errno));
                return false;
            }
        }

        if(set_nonblocking(clientfd) < 0) return false;

        _base->add_event(new streamio_event(clientfd, _ses->dup()), EVENT_RECV);
        TRACELOG("accept clientid=%d.\n", clientfd);
    }
    return 0;
}

activeio_event::activeio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_fd < 0)
    {
        perror("create socket");
        return;
    }
    set_nonblocking(_fd);
    struct sockaddr* addr = _ses->get_manager()->get_addr()->addr();
    if(connect(_fd, addr, sizeof(*addr)) < 0)
    {
        perror("connect");
        return;
    }
}

bool activeio_event::handle_event(int active_events)
{
    // TODO   
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
}

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_recv();
        _ses->permit_send();
    }
    else if(active_events & EVENT_SEND)
    {
        handle_send();
        _ses->permit_recv();
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
    int len = recv(_fd, rbuffer.end(), rbuffer.free_space(), 0);
    if(len > 0)
    {
        rbuffer.fast_resize(len);
        // 处理业务
        _ses->on_recv(len);
    }
    else if(len == 0)
    {
        close(_fd);
    }
    else
    {
        TRACELOG("recv[fd=%d] len=%d error[%d]:%s", _fd, len, errno, strerror(errno));
        close(_fd);
    }
#endif
    return len;
}

int streamio_event::handle_send()
{
    if(_base == nullptr) return -1;

    octets& wbuffer = _ses->wbuffer();
    int len = send(_fd, wbuffer.begin(), wbuffer.size(), 0);
    if(len >= 0)
    {
        wbuffer.erase(0, len);
        _ses->on_send(len);
    }
    else
    {
        _ses->permit_send();
    }
    return len;
}