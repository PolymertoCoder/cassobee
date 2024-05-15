#pragma once
#include <map>
#include "util.h"
#include "event.h"
#include "demultiplexer.h"

class demultiplexer;
using SIG_HANDLER = void(*)(int);

class epoller;

class reactor : public singleton_support<reactor>
{
public:
    using EVENTS_MAP = std::map<int, event*>;
    using TIMEREVENT_MAP = std::multimap<TIMETYPE, event*>;

    void init();
    int  run();
    void stop();
    int  add_event(event* ev, int events);
    void del_event(event* ev);

    bool handle_signal_event(int signum);
    void handle_timer_event();

    event* get_event(int fd);
    FORCE_INLINE bool& wakeup() { return _wakeup; }
    FORCE_INLINE bool use_timerevt() { return _use_timerevt; }

public:
    bool _stop = true;
    demultiplexer* _dispatcher;
    bool _wakeup = false;
    bool _use_timerevt = false;
    int  _timeout = 0;

    EVENTS_MAP _io_events;
    EVENTS_MAP _signal_events;
    TIMEREVENT_MAP _timer_events;
};

void set_signal(int signum, SIG_HANDLER handler);
void add_signal(int signum, bool(*callback)(int));