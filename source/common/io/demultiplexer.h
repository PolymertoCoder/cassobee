#pragma once
#include <set>
#include "event.h"

struct event;
class reactor;

class demultiplexer
{
public:
    virtual bool init() = 0;
    virtual int  add_event(event* ev, int events) = 0;
    virtual void del_event(event* ev) = 0;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) = 0;
    virtual void wakeup() = 0;

    bool& get_wakeup() { return _wakeup; }

protected:
    bool _wakeup = false;
    control_event* _ctrl_event = nullptr;
};

class epoller : public demultiplexer
{
public:
    virtual bool init() override;
    virtual int  add_event(event* ev, int events) override;
    virtual void del_event(event* ev) override;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) override;
    virtual void wakeup() override;

private:
    int _epfd;
    std::set<int> _listenfds;
};

