#pragma once
#include <map>
#include "../types.h"
#include "../util.h"
#include "event.h"
#include "demultiplexer.h"

class epoller;

class reactor : public singleton_support<reactor>
{
public:
    using EVENTS_MAP = std::map<int, event*>;
    
    reactor(epoller* ep) : _ep(ep) {}
    int run();
    int add_event(event* ev, int events);
    void del_event(event* ev);

    event* get_event(int fd);
public:
    epoller* _ep;
    EVENTS_MAP _ep_events;
};
