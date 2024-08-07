#pragma once
#include <set>

struct event;
class reactor;

class demultiplexer
{
public:
    virtual bool init() = 0;
    virtual int  add_event(event* ev, int events) = 0;
    virtual void del_event(event* ev) = 0;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) = 0;
};

class epoller : public demultiplexer
{
public:
    virtual bool init() override;
    virtual int  add_event(event* ev, int events) override;
    virtual void del_event(event* ev) override;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) override;

private:
    int _epfd;
    std::set<int> _listenfds;
};

