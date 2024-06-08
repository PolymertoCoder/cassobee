#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include "ioevent.h"

bool streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_ACCEPT)
    {
        handle_accept();
    }
    else if(active_events & EVENT_RECV)
    {
        handle_recv();
    }
    else if(active_events & EVENT_SEND)
    {
        handle_send();
    }
    return true;
}

int streamio_event::handle_accept()
{
    //printf("handle_accept begin.\n");
    if(_base == nullptr) return -1;

    struct sockaddr_in sock_client;
    socklen_t len = sizeof(sock_client);
    int clientfd = accept(_handle, (sockaddr*)&sock_client, &len);
    if(clientfd == -1)
    {
        if(errno != EAGAIN || errno != EINTR)
        {
            printf("accept: %s\n", strerror(errno));
            return -1;
        }
    }

    int ret = set_nonblocking(clientfd, true);
    if(ret < 0) return -1;

    event* ev = (event*)new streamio_event(clientfd);
    if(ev == nullptr) return -1;
    _base->add_event(ev, EVENT_RECV);
    printf("accept clientid=%d.\n", clientfd);

    return 0;
}

int streamio_event::handle_recv()
{
    if(_base == nullptr) return -1;

    memset(_readbuf, 0, READ_BUFFER_SIZE);
    _rlength = 0;

#if 0 //ET
    int idx = 0, len = 0;
    while(idx < READ_BUFFER_SIZE)
    {
        len = recv(_fd, _readbuf+idx, READ_BUFFER_SIZE-idx, 0);
        if(len > 0) {
            idx += len;
            printf("recv[%d]:%s\n", len, _readbuf);
        }
        else if(len == 0) {
            close(_fd);
        }
        else {
            close(_fd);
        }
    }

    if(idx == READ_BUFFER_SIZE && len != -1) {
        base->add_event(ev, EVENT_RECV); 
    } else if(len == 0) {
        //base->add_event(ev, EVENT_RECV); 
    } else {
        base->add_event(ev, EVENT_SEND);
    }

#else //LT
    int len = recv(_handle, _readbuf, READ_BUFFER_SIZE, 0);
    if(len > 0)
    {
        _rlength = len;
        _readbuf[len] = '\0';
        //TODO 处理业务
        
        //printf("recv[fd=%d]:%s\n", _fd, _readbuf);
        memcpy(_writebuf, _readbuf, _rlength);
        _wlength = _rlength;
        _base->add_event(this, EVENT_SEND);
        _rlength = 0;
    }
    else if(len == 0)
    {
        close(_handle);
    }
    else
    {
        printf("recv[fd=%d] len=%d error[%d]:%s\n", _handle, len, errno, strerror(errno));
        close(_handle);
    }
#endif
    return len;
}

int streamio_event::handle_send()
{
    if(_base == nullptr) return -1;

    int len = send(_handle, _writebuf, _wlength, 0);
    if(len >= 0)
    {
        if(len == _wlength)
        {
            _wlength = 0;
            _base->add_event(this, EVENT_RECV);
        }
        else
        {
            memcpy(_writebuf, _writebuf+len, _wlength-len);
            _wlength -= len;
            _base->add_event(this, EVENT_SEND);
        }
    }
    else
    {
        _base->add_event(this, EVENT_SEND);
    }
    return len;
}