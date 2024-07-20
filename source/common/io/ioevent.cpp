#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <cstring>

#include "ioevent.h"
#include "address.h"
#include "log.h"
#include "common.h"
#include "reactor.h"
#include "session.h"

passiveio_event::passiveio_event(int fd, address* local)
    : netio_event(fd), _local(local)
{
    
}

bool passiveio_event::handle_event(int active_events)
{
    if(_base == nullptr) return false;

    int family = _local->family();
    if(family == AF_INET)
    {
        struct sockaddr_in sock_client;
        socklen_t len = sizeof(sock_client);
        int clientfd = accept(_fd, (sockaddr*)&sock_client, &len);
        if(clientfd == -1)
        {
            if(errno != EAGAIN || errno != EINTR)
            {
                printf("accept: %s\n", strerror(errno));
                return false;
            }
        }

        int ret = set_nonblocking(clientfd, true);
        if(ret < 0) return false;

        address* peer = new ipv4_address(sock_client, len);
        event* ev = new streamio_event(clientfd, peer);
        _base->add_event(ev, EVENT_RECV);
        printf("accept clientid=%d.\n", clientfd);
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
                printf("accept: %s\n", strerror(errno));
                return false;
            }
        }

        int ret = set_nonblocking(clientfd, true);
        if(ret < 0) return false;
        address* peer = new ipv6_address(sock_client, len);
        event* ev = new streamio_event(clientfd, peer);
        _base->add_event(ev, EVENT_RECV);
        printf("accept clientid=%d.\n", clientfd);
    }
    return 0;
}

bool activeio_event::handle_event(int active_events)
{
    return true;
}

streamio_event::streamio_event(int fd, address* peer)
    : netio_event(fd)
{
    
}

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_recv();
    }
    else if(active_events & EVENT_SEND)
    {
        handle_send();
    }
    return true;
}

int streamio_event::handle_recv()
{
    if(_base == nullptr) return -1;

    memset(_ses->_readbuf, 0, READ_BUFFER_SIZE);
    _ses->_rlength = 0;

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
    int len = recv(_fd, _ses->_readbuf.data(), READ_BUFFER_SIZE, 0);
    if(len > 0)
    {
        _ses->_rlength = len;
        _ses->_readbuf.data()[len] = '\0';
        //TODO 处理业务
        
        //local_log("recv[fd=%d]:%s", _fd, _readbuf);
        memcpy(_ses->_writebuf.data(), _ses->_readbuf.data(), _ses->_rlength);
        _ses->_wlength = _ses->_rlength;
        _base->add_event(this, EVENT_SEND);
        _ses->_rlength = 0;
    }
    else if(len == 0)
    {
        close(_fd);
    }
    else
    {
        local_log("recv[fd=%d] len=%d error[%d]:%s", _fd, len, errno, strerror(errno));
        close(_fd);
    }
#endif
    return len;
}

int streamio_event::handle_send()
{
    if(_base == nullptr) return -1;

    int len = send(_fd, _ses->_writebuf.data(), _ses->_wlength, 0);
    if(len >= 0)
    {
        if(len == _ses->_wlength)
        {
            _ses->_wlength = 0;
            _base->add_event(this, EVENT_RECV);
        }
        else
        {
            memcpy(_ses->_writebuf.data(), _ses->_writebuf.data() + len, _ses->_wlength-len);
            _ses->_wlength -= len;
            _base->add_event(this, EVENT_SEND);
        }
    }
    else
    {
        _base->add_event(this, EVENT_SEND);
    }
    return len;
}