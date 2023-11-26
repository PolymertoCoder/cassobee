#pragma once
#include "reactor.h"

#define READ_BUFFER_SIZE 512
#define WRITE_BUFFER_SIZE 512

enum EVENT_MASK
{
    EVENT_NONE = 0,
    EVENT_ACCEPT = 0x01,
    EVENT_RECV = 0x02,
    EVENT_SEND = 0x04,
};

enum EVENT_STATUS
{
    EVENT_STATUS_NONE = 0,
    EVENT_STATUS_ADD = 1,
};

struct event
{
    int _fd;
    int _status;
    int _rlength;
    char _readbuf[READ_BUFFER_SIZE];
    int _wlength;
    char _writebuf[WRITE_BUFFER_SIZE];

    event() : _fd(-1), _status(EVENT_STATUS_NONE) {}
    event(int fd) : _fd(fd), _status(EVENT_STATUS_NONE) {}
    virtual ~event() {}
    int get_fd(){ return _fd; }
    int get_status(){ return _status; }
    void set_status(int status){ _status = status; }
    virtual int handle_accept(void* param) = 0;
    virtual int handle_recv(void* param) = 0;
    virtual int  handle_send(void* param) = 0;
};

struct common_event : event
{
    common_event(int fd) : event(fd) {}
    virtual int handle_accept(void* param) override;
    virtual int handle_recv(void* param) override;
    virtual int handle_send(void* param) override;
};
