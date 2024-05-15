#include "reactor.h"
#include "event.h"
#include "systemtime.h"
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
    }
    else if(events & EVENT_TIMER)
    {
        TIMETYPE expiretime = systemtime::get_time() + ev->get_handle();
        _timer_events.insert(std::make_pair(expiretime, ev));
    }
    else if(events & EVENT_SIGNAL)
    {
        _signal_events.emplace(ev->get_handle(), ev);
    }
    
    return _dispatcher->add_event(ev, events);
}

void reactor::del_event(event* ev)
{
    if(_dispatcher == nullptr || ev == nullptr) return;

    _dispatcher->del_event(ev);
    
    int fd = ev->get_handle();
    EVENTS_MAP::iterator itr = _io_events.find(fd);
    if(itr == _io_events.end()) return;

    event* temp = itr->second;
    _io_events.erase(itr);
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
    TIMETYPE nowtime = systemtime::get_time();
    std::multimap<TIMETYPE, timer_event*> changelist;
    for(auto iter = _timer_events.begin(); iter != _timer_events.end() && iter->first < nowtime;)
    {
        timer_event* tm = dynamic_cast<timer_event*>(iter->second);
        if(tm->handle_event(EVENT_TIMER))
        {
            if(tm->_repeats > 0){ --tm->_repeats; }
            if(tm->_repeats != 0)
            {
                changelist.insert(std::make_pair(nowtime+tm->_timeout, tm));
            }
        }
        else{ break; }
        iter = _timer_events.erase(iter);
    }
    for(auto& [expiretime, tm] : changelist)
    {
        _timer_events.insert(std::make_pair(expiretime, tm));
    }
}

void reactor::init()
{
    _dispatcher = new epoller();
    _dispatcher->init();
    _stop = false;
    _timeout = 10;
    add_event(new sigio_event(),   EVENT_RECV);
    add_event(new control_event(), EVENT_RECV);
}

int reactor::run()
{
    if(_dispatcher == nullptr) return -1;

    while(_stop)
    {
        _dispatcher->dispatch(this, _timeout);
        handle_timer_event();        
    }
    return 0;
}

void reactor::stop()
{
    _stop = true;
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
        printf("capture %d signal, but to deal with failure\n", signum);
    }
}

void add_signal(int signum, bool(*callback)(int))
{
    reactor::get_instance()->add_event(new signal_event(signum, callback), EVENT_SIGNAL);
}