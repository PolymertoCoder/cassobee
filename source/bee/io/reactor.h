#pragma once
#include <map>
#include <functional>

#include "common.h"
#include "event.h"
#include "lock.h"
#include "types.h"

class demultiplexer;
struct event;

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

    void add_signal(int signum, bool(*callback)(int));

    event* get_event(int fd);
    bool get_wakeup();
    FORCE_INLINE demultiplexer* get_dispatcher() const { return _dispatcher; }
    FORCE_INLINE bool use_timer_thread() { return _use_timer_thread; }

private:
    friend struct sigio_event;
    bool handle_signal_event(int signum);
    void handle_timer_event();

private:
    bool _stop = true;
    cassobee::rwlock _locker;
    demultiplexer* _dispatcher;
    bool _wakeup = true;
    bool _use_timer_thread = true;
    int  _timeout = -1; // ms

    EVENTS_MAP _io_events;
    EVENTS_MAP _signal_events;
    TIMEREVENT_MAP _timer_events;
};

std::thread start_threadpool_and_timer();

int add_timer(TIMETYPE timeout/*ms*/, std::function<bool()> handler);
int add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, std::function<bool(void*)> handler, void* param);
