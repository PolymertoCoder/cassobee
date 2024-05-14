#include <cerrno>
#include <cstdio>
#include <memory.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "reactor.h"
#include "util.h"
#include "event.h"

int control_event::_pipe[2] = {-1, -1};

control_event::control_event()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, control_event::_pipe) == -1)
    {
        printf("sigio_event create failed.\n");
        return;
    }
}

int control_event::handle_event(int active_events)
{
    char buf[32];
    size_t n = read(_pipe[0], buf, 1);
    if(n == 0 || n > 32 || !_base) return -1;
    return 0;
}

void control_event::wakeup(reactor* base)
{
    if(!base || base->wakeup()) return;
    write(_pipe[1], "0", 1);
    base->wakeup() = true;
}

int streamio_event::handle_event(int active_events)
{
    if(active_events & EVENT_ACCEPT)
    {
        return handle_accept();
    }
    else if(active_events & EVENT_RECV)
    {
        return handle_recv();
    }
    else if(active_events & EVENT_SEND)
    {
        return handle_send();
    }
    return 0;
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

timer_event::timer_event(int repeats, int timeout, callback cbk)
    : _repeats(repeats), _timeout(timeout), _callback(cbk)
{
}

int timer_event::handle_event(int active_events)
{
    if(!_callback(_param)) return -1;
    return 0;
}

int sigio_event::_pipe[2] = { -1, -1 };

sigio_event::sigio_event()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, sigio_event::_pipe) == -1)
    {
        printf("sigio_event create failed.\n");
        return;
    }
}

int sigio_event::handle_event(int active_events)
{
    //write(sigio_event::_pipe[1], (char*)&active_events, 1);
    return 0;
}

void sigio_event::sigio_callback(int signum)
{
    write(sigio_event::_pipe[1], (char*)&signum, 1);
}

signal_event::signal_event(int signum, signal_callback callback)
    : _callback(callback)
{
    set_signal(signum, sigio_event::sigio_callback);
}

int signal_event::handle_event(int active_events)
{
    char buf[32];
    size_t n = read(sigio_event::_pipe[0], buf, 32);
    if(n == 0 || n > 32 || !_base) return -1;
    for(size_t i = 0; i < n; ++i){ if(!_callback(buf[i])) return -1; }
    return 0;
}