#pragma once
#include "address.h"
#include "event.h"
#include "session.h"

struct netio_event : event
{
    netio_event(int fd) : _fd(fd) {}
    int _fd = -1;
};

struct passiveio_event : netio_event
{
    passiveio_event(int fd, address* local);
    virtual bool handle_event(int active_events) override;
    address* _local;
};

struct activeio_event : netio_event
{
    activeio_event(int fd);
    virtual bool handle_event(int active_events) override;
};

struct streamio_event : netio_event
{
    streamio_event(int fd, address* peer);
    virtual bool handle_event(int active_events) override;
    int handle_recv();
    int handle_send();

    session* _ses;
    int _rlength;
    char _readbuf[READ_BUFFER_SIZE];
    int _wlength;
    char _writebuf[WRITE_BUFFER_SIZE];
};