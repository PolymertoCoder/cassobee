#pragma once
#include "event.h"

class session;
class session_manager;

struct netio_event : event
{
    netio_event(session* ses) : _ses(ses) {}
    int _fd = -1;
    session* _ses;
};

struct passiveio_event : netio_event
{
    passiveio_event(session_manager* manager);
    virtual bool handle_event(int active_events) override;
};

struct activeio_event : netio_event
{
    activeio_event(int fd);
    virtual bool handle_event(int active_events) override;
};

struct streamio_event : netio_event
{
    streamio_event(int fd, session* ses);
    virtual bool handle_event(int active_events) override;
    int handle_recv();
    int handle_send();
};