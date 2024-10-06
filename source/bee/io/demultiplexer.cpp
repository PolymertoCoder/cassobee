#include <sys/epoll.h>
#include <cstdio>

#include "demultiplexer.h"
#include "log.h"
#include "event.h"
#include "macros.h"
#include "reactor.h"
#include "types.h"

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
        //event.events |= (EPOLLET | EPOLLOUT);
        _ctrl_event = dynamic_cast<control_event*>(ev);
        CHECK_BUG(_ctrl_event, );
        printf("add wakeup events\n");  
    }

    int op;
    if(ev->get_status() == EVENT_STATUS_NONE)
    {
        op = EPOLL_CTL_ADD;
        ev->set_status(EVENT_STATUS_ADD);
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
    printf("epoller::add_event success\n");
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
    _wakeup = false;
    struct epoll_event events[EPOLL_ITEM_MAX];
    int nready = epoll_wait(_epfd, events, EPOLL_ITEM_MAX, timeout);
    _wakeup = true;
    printf("epoller wakeup... timeout=%d nready=%d\n", timeout, nready);

    for(int i = 0; i < nready; i++)
    {
        event* ev = base->get_event(events[i].data.fd);
        if(ev == nullptr) continue;

        printf("event fd=%d events=%d\n", events[i].data.fd, events[i].events);
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
    printf("epoller::wakeup() begin _wakeup=%s\n", expr2boolstr(_wakeup));
    if(_ctrl_event == nullptr || _wakeup) return;
    this->add_event(_ctrl_event, EVENT_WAKEUP);
    write(_ctrl_event->_control_pipe[1], "\0", 1);
    _wakeup = false;
    printf("epoller::wakeup() run success\n");
}
