#include <print>
#include <sys/epoll.h>
#include <cstdio>

#include "demultiplexer.h"
#include "log.h"
#include "event.h"
#include "reactor.h"
#include "types.h"

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
        std::println("add accept event, listenfd is %d", ev->get_handle());
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
        // event.events |= (EPOLLET | EPOLLOUT);
    }

    int op;
    if(ev->get_status() == EVENT_STATUS_NONE)
    {
        op = EPOLL_CTL_ADD;
        ev->set_status(EVENT_STATUS_ADD);
        if(events & EVENT_WAKEUP)
        {
            _ctrl_event = dynamic_cast<control_event*>(ev);
            CHECK_BUG(_ctrl_event, );
            std::println("add wakeup events");  
        }
    }
    else if(ev->get_status() == EVENT_STATUS_ADD) // 已经加入过epoll
    {
        op = EPOLL_CTL_MOD;
    }
    else
    {
        std::println("add_event failed %d, epfd=%d", ev->get_handle(), _epfd);
        return -2;
    }

    if(int ret = epoll_ctl(_epfd, op, ev->get_handle(), &event))
    {
        std::println("add_event failed %d, ret=%d epfd=%d", ev->get_handle(), ret, _epfd);
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
        std::println("del_event failed %d", ev->get_handle());
        return;
    }
}

#define EPOLL_ITEM_MAX 1024

void epoller::dispatch(reactor* base, int timeout)
{
#ifdef _REENTRANT
    _wakeup.store(false);
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
#ifdef _REENTRANT
    _wakeup.store(true);
#else
    _wakeup = true;
#endif
    std::println("epoller wakeup... timeout=%d nready=%d", timeout, nready);

    for(int i = 0; i < nready; i++)
    {
        event* ev = base->get_event(events[i].data.fd);
        if(ev == nullptr || ev->get_status() != EVENT_STATUS_ADD) continue;

        //printf("epoller dispatch event fd=%d events=%d\n", events[i].data.fd, events[i].events);
        if(events[i].events & EPOLLIN || events[i].events & EPOLLHUP)
        {
            if(_listenfds.contains(events[i].data.fd))
            {
                ev->handle_event(EVENT_ACCEPT);
            }
            else
            {
                ev->handle_event(EVENT_RECV);
            }
        }
        if(events[i].events & EPOLLOUT)
        {
            ev->handle_event(EVENT_SEND);
        }
    }
}

void epoller::wakeup()
{
    std::println("epoller::wakeup() begin _wakeup=%s", expr2boolstr(_wakeup));
#ifdef _REENTRANT
    if(_ctrl_event == nullptr || _wakeup.exchange(true)) return;
#else
    if(_ctrl_event == nullptr || !_wakeup) return;
#endif
    // this->add_event(_ctrl_event, EVENT_WAKEUP);
    char dummy = 0;
    if(write(_ctrl_event->_control_pipe[1], &dummy, sizeof(dummy)) < 0)
    {
         perror("wakeup write failed");
    }
    std::println("epoller::wakeup() run success");
}
