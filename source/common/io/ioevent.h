#pragma once
#include "event.h"
#include "session.h"

struct netio_event : event
{
    netio_event(int fd) : event(fd) {}
    SID _sid;
    int _rlength;
    char _readbuf[READ_BUFFER_SIZE];
    int _wlength;
    char _writebuf[WRITE_BUFFER_SIZE];
};

struct passiveio_event : netio_event
{
    passiveio_event(int fd);
    virtual bool handle_event(int active_events) override;
};

struct activeio_event : netio_event
{
    activeio_event(int fd);
    virtual bool handle_event(int active_events) override;
};

struct streamio_event : netio_event
{
    streamio_event(int fd) : netio_event(fd) {}
    virtual bool handle_event(int active_events) override;
    int handle_accept();
    int handle_recv();
    int handle_send();

    session* _ses;
};