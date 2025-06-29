#pragma once
#include <map>
#include <set>
#include <functional>
#include <thread>

#include "double_buffer.h"
#include "event.h"
#include "load_balance.h"
#include "lock.h"
#include "runnable.h"
#include "types.h"

namespace bee
{

class demultiplexer;
struct event;

class reactor : public static_runnable
{
public:
    using EVENTS_MAP = std::map<int, event*>;
    using TIMEREVENT_MAP = std::multimap<TIMETYPE, event*>;

    ~reactor();
    void init();
    void run() override;
    void stop();
    void wakeup();
    void add_event(event* ev, bool dispatch = false);
    void del_event(event* ev);

    void add_signal(int signum, bool(*callback)(int));

    event* get_event(int fd);
    auto& get_wakeup();
    static reactor* get_instance();
    FORCE_INLINE demultiplexer* get_dispatcher() const { return _dispatcher; }
    FORCE_INLINE bool use_timer_thread() const { return _use_timer_thread; }
    FORCE_INLINE bool is_main_reactor() const { return this == get_instance(); }

    FORCE_INLINE bool is_io_events(int events)
    {
        return events & EVENT_ACCEPT || events & EVENT_RECV   || events & EVENT_SEND ||
               events & EVENT_HUP    || events & EVENT_WAKEUP;
    }
    FORCE_INLINE bool is_timer_events(int events)
    {
        return events & EVENT_TIMER;
    }
    FORCE_INLINE bool is_signal_events(int events)
    {
        return events & EVENT_SIGNAL;
    }

private:
    friend struct sigio_event;
    void load_event();
    int  add_event_inner(event* ev);
    void del_event_inner(event* ev);

    bool handle_signal_event(int signum);
    void handle_timer_event();

private:
    struct sub_reactor
    {
        reactor* impl = nullptr;
        std::thread* th = nullptr;
    };
    void create_load_balancer();

private:
    static reactor* _instance;
    static bee::mutex _instance_mutex;

    // 主reactor才有的数据
    std::vector<sub_reactor> _sub_reactors;
    std::atomic<int> _next_index{0}; // 轮询索引;
    LOAD_BALANCE_STRATEGY _strategy = ROUND_ROBIN;
    load_balancer<sub_reactor>* _balancer = nullptr;

    bool _stop = true;
    demultiplexer* _dispatcher;
    bool _wakeup = true;
    bool _use_timer_thread = true;
    int  _timeout = -1; // ms

    bee::one_reader_double_buffer<event*, std::set> _changelist;

    EVENTS_MAP _io_events;
    EVENTS_MAP _signal_events;
    TIMEREVENT_MAP _timer_events;
};

std::thread start_threadpool_and_timer();

TIMERID add_timer(TIMETYPE timeout/*ms*/, std::function<bool()> handler);
TIMERID add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, std::function<bool()> handler);
TIMERID add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, std::function<bool(void*)> handler, void* param);

void del_timer(TIMERID timerid);

} // namespace bee