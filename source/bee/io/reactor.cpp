#include "reactor.h"

#include <map>
#include <utility>
#include <sys/signal.h>

#include "glog.h"
#include "event.h"
#include "lock.h"
#include "log.h"
#include "systemtime.h"
#include "timewheel.h"
#include "demultiplexer.h"
#include "threadpool.h"
#include "types.h"
#include "config.h"

namespace bee
{

reactor* reactor::_instance = nullptr;
bee::mutex reactor::_instance_mutex;

reactor::~reactor()
{
    stop();

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
    for(auto data : _sub_reactors)
    {
        if(data.th->joinable())
        {
            data.th->join();
        }
        delete data.th;
        delete data.impl;
    }
}

void reactor::init()
{
    auto cfg = config::get_instance();
    if(cfg->get("reactor", "demultiplexer") == "epoller")
    {
        _dispatcher = new epoller();
    }
    ASSERT(_dispatcher);
    _dispatcher->init();
    _use_timer_thread = cfg->get<bool>("reactor", "use_timer_thread");
    if(!_use_timer_thread)
    {
        _timeout = cfg->get<int>("reactor", "timeout");
    }

    set_signal(SIGPIPE, SIG_IGN);
    add_event(new sigio_event());
    add_event(new control_event());

    if(is_main_reactor())
    {
        size_t sub_reactor_count = cfg->get<size_t>("reactor", "sub_reactor_count", 0);
        for(size_t idx = 0; idx < sub_reactor_count; ++idx)
        {
            sub_reactor data;
            data.impl = new reactor;
            data.impl->init();
            data.th = new std::thread([impl = data.impl]()
            {
                impl->run();
                impl->destroy();
            });
        }
    }
}

void reactor::run()
{
    if(_dispatcher == nullptr) return;
    _stop = false;

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
    }
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

void reactor::add_event(event* ev, bool dispatch)
{
    // if(dispatch)
    // {

    //     return;
    // }
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

auto& reactor::get_wakeup()
{
    return _dispatcher->get_wakeup();
}

reactor* reactor::get_instance()
{
    if(_instance == nullptr)
    {
        bee::mutex::scoped l(_instance_mutex);
        if(_instance == nullptr) _instance = new reactor;
    }
    return _instance;
}

void reactor::load_event()
{
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
    if(is_io_events(events))
    {
        int handle = ev->get_handle();
        if(auto iter = _io_events.find(handle); iter != _io_events.end() && iter->second != ev)
        {
            delete iter->second;
            local_log("new event %p substitute old event %p", (void*)ev, (void*)iter->second);
        }
        _io_events[handle] = ev;
        if(int ret = _dispatcher->add_event(ev, events))
        {
            local_log("add_io_event error, ret=%d.", ret);
            return ret;
        }
        //TRACELOG("add_io_event handle=%d events=%d.", ev->get_handle(), events);
    }
    else if(is_timer_events(events))
    {
        timer_event* tm = dynamic_cast<timer_event*>(ev);
        if(tm == nullptr)
        {
            ERRORLOG("reactor add_event EVENT_TIMER is not timer_event.");
            return -1;
        }
        TIMETYPE nowtime = systemtime::get_millseconds();
        TIMETYPE expiretime = tm->_delay ? (nowtime + tm->_timeout) : nowtime;
        tm->_delay = true;
        _timer_events.insert(std::make_pair(expiretime, ev));
        //local_log("add_timer_event nowtime=%ld delay=%d timeout=%ld expiretime=%ld.", nowtime, tm->_delay, tm->_timeout, expiretime);
    }
    else if(is_signal_events(events))
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

    if(is_io_events(ev->get_events()))
    {
        _dispatcher->del_event(ev);
        
        int fd = ev->get_handle();
        if(auto iter = _io_events.find(fd); iter != _io_events.end())
        {
            delete iter->second;
            _io_events.erase(iter);
        }
        local_log("reactor del_event fd=%d.", fd);
    }
    else if(is_timer_events(ev->get_events()))
    {
        local_log("reactor timer_event %p will be deleted timer when actually run instead of immediately.", ev);
    }
    else if(is_signal_events(ev->get_events()))
    {
        _signal_events.erase(ev->get_handle());
    }
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
        timer_event* tm = static_cast<timer_event*>(ev);
        if(expiretime <= nowtime) break;

        if(tm->get_status() == EVENT_STATUS_DEL)
        {
            delete tm;
            local_log("handle_timer_event delete timer event.");
            continue;
        }
        if(tm->handle_event(EVENT_TIMER))
        {
            add_event(tm);
        }
    }
}

void reactor::create_load_balancer()
{
    load_balance_strategy<sub_reactor>* strategy = nullptr;
    switch(_strategy)
    {
        case LOAD_BALANCE_STRATEGY::ROUND_ROBIN:
        {
            strategy = load_balance_strategy_factory<sub_reactor>::create("round_robin_strategy<sub_reactor>");
        }
        case LOAD_BALANCE_STRATEGY::RANDOM:
        {
            strategy = load_balance_strategy_factory<sub_reactor>::create("random_strategy<sub_reactor>");
        } break;
        case LOAD_BALANCE_STRATEGY::LEAST_CONNECTIONS:
        {
            strategy = load_balance_strategy_factory<sub_reactor>::create("least_connection_strategy<sub_reactor>");
        } break;
        default: { break; }
    }
    _balancer = new load_balancer<sub_reactor>(std::unique_ptr<load_balance_strategy<sub_reactor>>(strategy));
}

std::thread start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    if(reactor::get_instance()->use_timer_thread())
    {
        return std::thread([]()
        {
            local_log("timer thread tid:%d.", gettid());
            timewheel::get_instance()->init();
            timewheel::get_instance()->run();
        });
    }
    return {};
}

TIMERID add_timer(TIMETYPE timeout, std::function<bool()> handler)
{
    return add_timer(true, timeout, -1, [handler](void*){ return handler(); }, nullptr);
}

TIMERID add_timer(bool delay, TIMETYPE timeout, int repeats, std::function<bool()> handler)
{
    return add_timer(delay, timeout, repeats, [handler](void*){ return handler(); }, nullptr);
}

TIMERID add_timer(bool delay, TIMETYPE timeout, int repeats, std::function<bool(void*)> handler, void* param)
{
    if(reactor::get_instance()->use_timer_thread())
    {
        return timewheel::get_instance()->add_timer(delay, timeout, repeats, handler, param);
    }
    else
    {
        auto* tm = new timer_event(delay, timeout, repeats, handler, param);
        reactor::get_instance()->add_event(tm);
        return reinterpret_cast<TIMERID>(tm);
    }
}

void del_timer(TIMERID timerid)
{
    if(reactor::get_instance()->use_timer_thread())
    {
        timewheel::get_instance()->del_timer(timerid);
    }
    else
    {
        if(timerid <= 0) return;
        reactor::get_instance()->del_event(reinterpret_cast<event*>(timerid));
    }
}

} // namespace bee