#include "reactor.h"

#include <filesystem>
#include <map>
#include <utility>

#include "event.h"
#include "lock.h"
#include "systemtime.h"
#include "timewheel.h"
#include "log.h"
#include "demultiplexer.h"
#include "threadpool.h"
#include "types.h"
#include "config.h"

reactor::~reactor()
{
    delete _dispatcher;
    for(auto& [fd, evt]: _io_events)
    {
        if(evt->is_close()) continue;
        delete evt;
    }
    for(auto& [signum, evt]: _signal_events)
    {
        if(evt->is_close()) continue;
        delete evt;
    }
    for(auto& [timeout, evt]: _timer_events)
    {
        if(evt->is_close()) continue;
        delete evt;
    }
}

void reactor::init()
{
    auto cfg = config::get_instance();
    if(cfg->get("reactor", "demultiplexer") == "epoller")
    {
        _dispatcher = new epoller();
    }
    _dispatcher->init();
    _stop = false;
    _use_timer_thread = cfg->get<bool>("reactor", "use_timer_thread");
    if(!_use_timer_thread)
    {
        _timeout = cfg->get<int>("reactor", "timeout");
    }

    add_event(new sigio_event());
    add_event(new control_event());
}

int reactor::run()
{
    if(_dispatcher == nullptr) return -1;

    while(!_stop)
    {
        load_event();
        int timeout = _timeout;
        if(!use_timer_thread() && _timer_events.size())
        {
            TIMETYPE nowtime = systemtime::get_millseconds();
            TIMETYPE diff = _timer_events.begin()->first - nowtime;
            if(diff > _timeout) timeout = diff;
            //local_log("reactor::run nexttime=%ld nowtime=%ld timeout=%d", _timer_events.begin()->first, nowtime, timeout);
        }
        _dispatcher->dispatch(this, timeout);
        handle_timer_event();
        //local_log("reactor::run end\n");
    }
    return 0;
}

void reactor::stop()
{
    _stop = true;
    wakeup();
}

void reactor::wakeup()
{
    _dispatcher->wakeup();
}

void reactor::add_event(event* ev)
{
    _changelist.write(ev);
    wakeup();
}

void reactor::del_event(event* ev)
{
    ev->set_status(EVENT_STATUS_DEL);
    _changelist.write(ev);
    wakeup();
}

void reactor::add_signal(int signum, bool(*callback)(int))
{
    auto evt = new signal_event(signum, callback);
    evt->set_events(EVENT_SIGNAL);
    add_event(evt);
}

event* reactor::get_event(int fd)
{
    EVENTS_MAP::iterator itr = _io_events.find(fd);
    return itr != _io_events.end() ? itr->second : nullptr;
}

bool& reactor::get_wakeup()
{
    return _dispatcher->get_wakeup();
}

void reactor::load_event()
{
    get_wakeup() = true;
    using changelist_type = decltype(_changelist)::list_type;
    _changelist.read([this](changelist_type& list)
    {
        for(const auto& evt : list)
        {
            if(evt->get_status() != EVENT_STATUS_DEL)
            {
                add_event_inner(evt);
            }
            else
            {
                del_event_inner(evt);
            }
        }
    });
}

int reactor::add_event_inner(event* ev)
{
    if(_dispatcher == nullptr || ev == nullptr) return -1;

    int events = ev->get_events();
    if(events & EVENT_ACCEPT || events & EVENT_RECV || events & EVENT_SEND || events & EVENT_HUP || events & EVENT_WAKEUP)
    {
        _io_events[ev->get_handle()] = ev;
        if(int ret = _dispatcher->add_event(ev, events))
        {
            local_log("add_io_event error, ret=%d.", ret);
            return ret;
        }
        //TRACELOG("add_io_event handle=%d events=%d.", ev->get_handle(), events);
    }
    else if(events & EVENT_TIMER)
    {
        timer_event* tm = dynamic_cast<timer_event*>(ev);
        TIMETYPE nowtime = systemtime::get_millseconds();
        TIMETYPE expiretime = tm->_delay ? (nowtime + tm->_timeout) : nowtime;
        tm->_delay = true;
        _timer_events.insert(std::make_pair(expiretime, ev));
        //local_log("add_timer_event nowtime=%ld delay=%d timeout=%ld expiretime=%ld.", nowtime, tm->_delay, tm->_timeout, expiretime);
    }
    else if(events & EVENT_SIGNAL)
    {
        _signal_events.emplace(ev->get_handle(), ev);
        //local_log("add_signal_event signum=%d.", ev->get_handle());
    }
    else
    {
        CHECK_BUG(false, ERRORLOG("reactor add_event unknown events:%d.", events); return -2);
    }
    ev->_base = this;
    //local_log("reactor::add_event fd=%d events=%d\n", ev->get_handle(), events);
    return 0;
}

void reactor::del_event_inner(event* ev)
{
    if(_dispatcher == nullptr || ev == nullptr) return;

    _dispatcher->del_event(ev);
    
    int fd = ev->get_handle();
    if(auto iter = _io_events.find(fd); iter != _io_events.end())
    {
        delete iter->second;
        _io_events.erase(iter);
    }
    local_log("reactor del_event fd=%d.\n", fd);
}

bool reactor::handle_signal_event(int signum)
{
    local_log("handle_signal_event run.");
    if(auto iter = _signal_events.find(signum); iter != _signal_events.end())
    {
        local_log("handle_signal_event run inner.");
        return iter->second->handle_event(EVENT_SIGNAL);
    }
    return true;
}

void reactor::handle_timer_event()
{
    if(use_timer_thread()) return;
    TIMETYPE nowtime = systemtime::get_millseconds();
    while(_timer_events.size())
    {
        //local_log("handle_timer_event expiretime=%ld nowtime=%ld end", iter->first, nowtime);
        auto [expiretime, ev] = *(_timer_events.begin());
        _timer_events.erase(_timer_events.begin());
        timer_event* tm = dynamic_cast<timer_event*>(ev);
        if(expiretime <= nowtime) break;
        if(tm->handle_event(EVENT_TIMER))
        {
            add_event(tm);
        }
    }
}

std::thread start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    if(reactor::get_instance()->use_timer_thread())
    {
        return std::thread([]()
        {
            printf("timer thread tid:%d.\n", gettid());
            timewheel::get_instance()->init();
            timewheel::get_instance()->run();
        });
    }
    return {};
}

int add_timer(TIMETYPE timeout, std::function<bool()> handler)
{
    return add_timer(true, timeout, -1, [handler](void*){ return handler(); }, nullptr);
}

int add_timer(bool delay, TIMETYPE timeout, int repeats, std::function<bool(void*)> handler, void* param)
{
    if(reactor::get_instance()->use_timer_thread())
    {
        return timewheel::get_instance()->add_timer(delay, timeout, repeats, handler, param);
    }
    else
    {
        reactor::get_instance()->add_event(new timer_event(delay, timeout, repeats, handler, param));
        return 0;
    }
}