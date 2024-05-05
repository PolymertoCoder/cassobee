#pragma once
#include <map>
#include "util.h"
#include "event.h"
#include "demultiplexer.h"

class epoller;

class reactor : public singleton_support<reactor>
{
public:
    using EVENTS_MAP = std::map<int, event*>;
    using TIMEEVENT_MAP = std::map<int, timer_event*>;
    
    reactor(epoller* ep) : _ep(ep) {}
    int run();
    int add_event(event* ev, int events);
    void del_event(event* ev);

    event* get_event(int fd);
    bool use_timerevt() { return _use_timerevt; }
public:
    epoller* _ep;
    EVENTS_MAP _ep_events;
    bool _use_timerevt;
    TIMEEVENT_MAP _timer_events;
};
