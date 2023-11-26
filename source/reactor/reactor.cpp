#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "reactor.h"

int reactor::add_event(event* ev, int events)
{
    if(_ep == nullptr || ev == nullptr) return -1;

    EVENTS_MAP::iterator itr = _ep_events.find(ev->get_fd());
    if(itr == _ep_events.end())
    {
        _ep_events.emplace(ev->get_fd(), ev);
    }
    return _ep->add_event(ev, events);
}

void reactor::del_event(event* ev)
{
    if(_ep == nullptr || ev == nullptr) return;

    _ep->del_event(ev);
    
    int fd = ev->get_fd();
    EVENTS_MAP::iterator itr = _ep_events.find(fd);
    if(itr == _ep_events.end()) return;

    event* temp = itr->second;
    _ep_events.erase(itr);
    delete temp;
}

int reactor::run()
{
    if(_ep == nullptr) return -1;

    while(true)
    {
        _ep->dispatch(this);
    }
}

event* reactor::get_event(int fd)
{
    EVENTS_MAP::iterator itr = _ep_events.find(fd);
    return itr != _ep_events.end() ? itr->second : nullptr;
}
