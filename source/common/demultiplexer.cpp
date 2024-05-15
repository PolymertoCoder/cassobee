#include <cstdio>
#include <sys/epoll.h>
#include "demultiplexer.h"

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
        printf("add accept event, listenfd is %d\n", ev->get_handle());
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
    if(events & EVENT_TIMER)
    {
        event.events |= EVENT_TIMER;
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
        printf("add_event failed %d, epfd=%d\n", ev->get_handle(), _epfd);
        return -2;
    }

    int ret = epoll_ctl(_epfd, op, ev->get_handle(), &event);
    if(ret < 0)
    {
        printf("add_event failed %d, ret=%d epfd=%d\n", ev->get_handle(), ret, _epfd);
        return -3;
    }
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
        printf("del_event failed %d\n", ev->get_handle());
        return;
    }
}

#define EPOLL_ITEM_MAX 10240

void epoller::dispatch(reactor* base, int timeout)
{
    struct epoll_event events[EPOLL_ITEM_MAX];
    //int checkpos = 0;
    int nready = epoll_wait(_epfd, events, EPOLL_ITEM_MAX, timeout);

    for(int i = 0; i < nready; i++)
    {
        event* ev = base->get_event(events[i].data.fd);
        if(ev == nullptr) continue;

        if(events[i].events & EPOLLIN || events[i].events & EPOLLOUT || events[i].events & EPOLLHUP)
        {
            if(_listenfds.count(events[i].data.fd)) {
                ev->handle_event(EVENT_ACCEPT);
            } else {
                ev->handle_event(EVENT_RECV);
            }
        }
        if(events[i].events & EPOLLOUT)
        {
            ev->handle_event(EVENT_SEND);
        }
    }
}