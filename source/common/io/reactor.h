#pragma once
#include <map>
#include <functional>
#include "common.h"
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
    void wakeup();
    int  add_event(event* ev, int events);
    void del_event(event* ev);

    bool handle_signal_event(int signum);
    void handle_timer_event();

    event* get_event(int fd);
    FORCE_INLINE bool& get_wakeup() { return _wakeup; }
    FORCE_INLINE bool use_timer_thread() { return _use_timer_thread; }

public:
    bool _stop = true;
    demultiplexer* _dispatcher;
    bool _wakeup = false;
    bool _use_timer_thread = true;
    int  _timeout = 0; // ms

    EVENTS_MAP _io_events;
    EVENTS_MAP _signal_events;
    TIMEREVENT_MAP _timer_events;
};

void set_signal(int signum, SIG_HANDLER handler);
void add_signal(int signum, bool(*callback)(int));

int add_timer(TIMETYPE timeout/*ms*/, std::function<bool()> handler);
int add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, std::function<bool(void*)> handler, void* param);