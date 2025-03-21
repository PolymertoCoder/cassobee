#pragma once
#include <map>
#include <set>
#include <functional>
#include <thread>

#include "cc_changelist.h"
#include "common.h"
#include "event.h"
#include "types.h"

namespace bee
{

class demultiplexer;
struct event;

class reactor : public singleton_support<reactor>
{
public:
    using EVENTS_MAP = std::map<int, event*>;
    using TIMEREVENT_MAP = std::multimap<TIMETYPE, event*>;

    ~reactor();
    void init();
    int  run();
    void stop();
    void wakeup();
    void add_event(event* ev);
    void del_event(event* ev);

    void add_signal(int signum, bool(*callback)(int));

    event* get_event(int fd);
    auto& get_wakeup();
    FORCE_INLINE demultiplexer* get_dispatcher() const { return _dispatcher; }
    FORCE_INLINE bool use_timer_thread() { return _use_timer_thread; }

private:
    friend struct sigio_event;
    void load_event();
    int  add_event_inner(event* ev);
    void del_event_inner(event* ev);

    bool handle_signal_event(int signum);
    void handle_timer_event();

private:
    bool _stop = true;
    demultiplexer* _dispatcher;
    bool _wakeup = true;
    bool _use_timer_thread = true;
    int  _timeout = -1; // ms

    bee::cc_changelist<event*, std::set> _changelist;

    EVENTS_MAP _io_events;
    EVENTS_MAP _signal_events;
    TIMEREVENT_MAP _timer_events;
};

std::thread start_threadpool_and_timer();

int add_timer(TIMETYPE timeout/*ms*/, std::function<bool()> handler);
int add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, std::function<bool(void*)> handler, void* param);

} // namespace bee