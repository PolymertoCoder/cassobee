#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "glog.h"
#include "demultiplexer.h"
#include "event.h"
#include "reactor.h"

namespace bee
{

epoller::~epoller()
{
    if(_epfd >= 0)
    {
        close(_epfd);
    }
}

bool epoller::init()
{
    _epfd = epoll_create(1);
    if(_epfd < 0)
    {
        perror("epoll_create");
        return false;
    }
    return true;
}

int epoller::add_event(event* ev, int events)
{
    if(events == EVENT_NONE) return -1;

    struct epoll_event event = {0, {0}};
    event.data.ptr = ev;
    event.data.fd = ev->get_handle();

    if(events & EVENT_ACCEPT)
    {
        event.events |= EPOLLIN;
        _listenfds.insert(ev->get_handle());
        local_log("add accept event, listenfd is %d", ev->get_handle());
    }
    if(events & EVENT_RECV)
    {
        event.events |= EPOLLIN;
    }
    if(events & EVENT_SEND)
    {
        event.events |= EPOLLOUT;
    }
    if(events & EVENT_HUP)
    {
        event.events |= EPOLLHUP;
    }
    if(events & EVENT_WAKEUP)
    {
        event.events |= EPOLLIN;
    }

    int op;
    if(ev->get_status() == EVENT_STATUS_NONE)
    {
        op = EPOLL_CTL_ADD;
        ev->set_status(EVENT_STATUS_ADD);
        if(events & EVENT_WAKEUP)
        {
            _wakeup_fd = ev->get_handle();
            local_log("set wakeupfd:%d.", _wakeup_fd);  
        }
    }
    else if(ev->get_status() == EVENT_STATUS_ADD) // 已经加入过epoll
    {
        op = EPOLL_CTL_MOD;
    }
    else
    {
        local_log("add_event failed %d, epfd=%d", ev->get_handle(), _epfd);
        return -2;
    }

    if(int ret = epoll_ctl(_epfd, op, ev->get_handle(), &event))
    {
        local_log("add_event failed %d, ret=%d epfd=%d", ev->get_handle(), ret, _epfd);
        return -3;
    }
    //printf("epoller::add_event success handle=%d events=%d\n", ev->get_handle(), events);
    return 0;
}

void epoller::del_event(event* ev)
{
    if(ev->get_status() != EVENT_STATUS_ADD) return;
    ev->set_status(EVENT_STATUS_NONE);

    struct epoll_event event = {0, {0}};
    event.data.ptr = ev;

    if(epoll_ctl(_epfd, EPOLL_CTL_DEL, ev->get_handle(), &event) < 0)
    {
        local_log("del_event failed %d", ev->get_handle());
        return;
    }
}

#define EPOLL_ITEM_MAX 1024

void epoller::dispatch(reactor* base, int timeout)
{
#ifdef _REENTRANT
    _wakeup.store(false, std::memory_order_release);
#else
    _wakeup = false;
#endif
    struct epoll_event events[EPOLL_ITEM_MAX];
    int nready = epoll_wait(_epfd, events, EPOLL_ITEM_MAX, timeout);
    if(nready < 0)
    {
        if(errno == EINTR) return; // 信号中断，忽略
        perror("epoll_wait");
        return;
    }
    local_log("epoller wakeup... timeout=%d nready=%d", timeout, nready);

    for(int i = 0; i < nready; i++)
    {
        event* ev = base->get_event(events[i].data.fd);
        if(ev == nullptr || ev->get_status() != EVENT_STATUS_ADD) continue;

        //printf("epoller dispatch event fd=%d events=%d\n", events[i].data.fd, events[i].events);
        int active_events = 0;
        if(events[i].events & EPOLLIN || events[i].events & EPOLLHUP)
        {
            if(_listenfds.contains(events[i].data.fd))
            {
                active_events |= EVENT_ACCEPT;
            }
            else
            {
                active_events |= EVENT_RECV;
            }
        }
        if(events[i].events & EPOLLOUT)
        {
            active_events |= EVENT_SEND;
        }
        ev->handle_event(active_events);
    }
}

void epoller::wakeup()
{
    if(_wakeup_fd < 0) return;
    // local_log("epoller::wakeup() begin _wakeup=%s", expr2boolstr(_wakeup));
#ifdef _REENTRANT
    if(_wakeup.exchange(true, std::memory_order_acq_rel)) return;
#else
    if(_wakeup) return;
    _wakeup = true;
#endif
    static constexpr eventfd_t value = 1;
    if(write(_wakeup_fd, &value, sizeof(value)) < 0)
    {
        if (errno != EAGAIN) perror("wakeupfd write failed");
    }
    // local_log("epoller::wakeup() run success");
}

} // namespace bee