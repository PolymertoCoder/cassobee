#include "reactor.h"
#include "event.h"
#include "systemtime.h"
#include "timewheel.h"
#include "log.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <map>
#include <sys/epoll.h>
#include <utility>

int reactor::add_event(event* ev, int events)
{
    if(_dispatcher == nullptr || ev == nullptr) return -1;

    if(events & EVENT_ACCEPT || events & EVENT_RECV || events & EVENT_SEND || events & EVENT_HUP)
    {
        _io_events.emplace(ev->get_handle(), ev);
        return _dispatcher->add_event(ev, events);
    }
    else if(events & EVENT_TIMER)
    {
        timer_event* tm = dynamic_cast<timer_event*>(ev);
        TIMETYPE nowtime = systemtime::get_millseconds();
        TIMETYPE expiretime = tm->_delay ? (nowtime + tm->_timeout) : nowtime;
        tm->_delay = true;
        _timer_events.insert(std::make_pair(expiretime, ev));
        local_log("add_timer_event nowtime=%ld delay=%d timeout=%ld expiretime=%ld.", nowtime, tm->_delay, tm->_timeout, expiretime);
    }
    else if(events & EVENT_SIGNAL)
    {
        _signal_events.emplace(ev->get_handle(), ev);
    }
    wakeup();
    return 0;
}

void reactor::del_event(event* ev)
{
    if(_dispatcher == nullptr || ev == nullptr) return;

    _dispatcher->del_event(ev);
    
    int fd = ev->get_handle();
    auto iter = _io_events.find(fd);
    if(iter == _io_events.end()) return;

    event* temp = iter->second;
    _io_events.erase(iter);
    delete temp;
}

bool reactor::handle_signal_event(int signum)
{
    if(auto iter = _signal_events.find(signum); iter != _signal_events.end())
    {
        return iter->second->handle_event(EVENT_SIGNAL);
    }
    return true;
}

void reactor::handle_timer_event()
{
    TIMETYPE nowtime = systemtime::get_millseconds();
    while(_timer_events.size())
    {
        //local_log("handle_timer_event expiretime=%ld nowtime=%ld end", iter->first, nowtime);
        auto& [expiretime, ev] = *(_timer_events.begin());
        _timer_events.erase(_timer_events.begin());
        timer_event* tm = dynamic_cast<timer_event*>(ev);
        if(expiretime <= nowtime) break;
        if(tm->handle_event(EVENT_TIMER))
        {
            add_event(tm, EVENT_TIMER);
        }
    }
}

void reactor::init()
{
    _dispatcher = new epoller();
    _dispatcher->init();
    _stop = false;
    _use_timer_thread = true;
    _timeout = 1;
    add_event(new sigio_event(),   EVENT_RECV);
    add_event(new control_event(), EVENT_RECV);
}

int reactor::run()
{
    if(_dispatcher == nullptr) return -1;

    while(!_stop)
    {
        int timeout = _timeout;
        if(_timer_events.size())
        {
            TIMETYPE nowtime = systemtime::get_millseconds();
            TIMETYPE diff = _timer_events.begin()->first - nowtime;
            if(diff > _timeout) timeout = diff;
            //local_log("reactor::run nexttime=%ld nowtime=%ld timeout=%d", _timer_events.begin()->first, nowtime, timeout);
        }
        _dispatcher->dispatch(this, timeout);
        handle_timer_event();        
    }
    return 0;
}

void reactor::stop()
{
    _stop = true;
}

void reactor::wakeup()
{
    control_event::wakeup(this);
}

event* reactor::get_event(int fd)
{
    EVENTS_MAP::iterator itr = _io_events.find(fd);
    return itr != _io_events.end() ? itr->second : nullptr;
}

void set_signal(int signum, SIG_HANDLER handler)
{
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = handler; // 设置信号处理函数
    sigfillset(&act.sa_mask); // 初始化信号屏蔽集
    act.sa_flags |= SA_RESTART; // 由此信号中断的系统调用自动重启动

    if(sigaction(signum, &act, NULL) == -1)
    {
        local_log("capture %d signal, but to deal with failure", signum);
    }
}

void add_signal(int signum, bool(*callback)(int))
{
    reactor::get_instance()->add_event(new signal_event(signum, callback), EVENT_SIGNAL);
}

int add_timer(TIMETYPE timeout, std::function<void()> handler)
{
    return add_timer(true, timeout, -1, [handler](void*){ handler(); return false; }, nullptr);
}

int add_timer(bool delay, TIMETYPE timeout, int repeats, std::function<bool(void*)> handler, void* param)
{
    if(reactor::get_instance()->use_timer_thread())
    {
        return timewheel::get_instance()->add_timer(delay, timeout, repeats, handler, param);
    }
    else
    {
        return reactor::get_instance()->add_event(new timer_event(delay, timeout, repeats, handler, param), EVENT_TIMER);
    }
}