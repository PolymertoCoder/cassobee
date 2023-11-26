#include <sys/epoll.h>
#include "demultiplexer.h"

int epoller::add_event(event* ev, int events)
{
    if(events == EVENT_NONE) return -1;

    struct epoll_event event = {0, {0}};
    event.data.ptr = ev;
    event.data.fd = ev->get_fd();

    if(events & EVENT_ACCEPT)
    {
        event.events |= EPOLLIN;
        _listenfd = ev->get_fd();
        printf("listenfd is %d\n", _listenfd);
    }

    if(events & EVENT_RECV)
    {
        event.events |= EPOLLIN;
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

    if(epoll_ctl(_epfd, op, ev->get_fd(), &event) < 0)
    {
        printf("add_event failed %d, epfd=%d\n", ev->get_fd(), _epfd);
        return -1;
    }
    return 0;
}

void epoller::del_event(event* ev)
{
    if(ev->get_status() != EVENT_STATUS_ADD) return;
    ev->set_status(EVENT_STATUS_NONE);

    struct epoll_event event = {0, {0}};
    event.data.ptr = ev;

    if(epoll_ctl(_epfd, EPOLL_CTL_DEL, ev->get_fd(), &event) < 0)
    {
        printf("del_event failed %d\n", ev->get_fd());
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

        if(events[i].events & EPOLLIN)
        {    
            if(_listenfd == events[i].data.fd) {
                ev->handle_accept((void*)base);
            } else {
                ev->handle_recv((void*)base);
            }
        }
        if(events[i].events & EPOLLOUT)
        {
            ev->handle_send((void*)base);
        }
    }
}
