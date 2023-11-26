#pragma once
#include<set>
#include "reactor.h"
#include "event.h"

struct event;
class reactor;

class demultiplexer
{
public: 
    virtual int add_event(event* ev, int events) = 0;
    virtual void del_event(event* ev) = 0;
    virtual void dispatch(reactor* base, int timeout = 1000) = 0;
};

class epoller : public demultiplexer
{
public:
    bool init();
    virtual int add_event(event* ev, int events) override;
    virtual void del_event(event* ev) override;
    virtual void dispatch(reactor* base, int timeout = 1000) override;

private:
    int _epfd;
    std::set<int> _listenfds;
};

