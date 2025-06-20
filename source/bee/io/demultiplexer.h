#pragma once
#ifdef _REENTRANT
#include <atomic>
#endif
#include <set>
#include "event.h"

namespace bee
{

struct event;
class reactor;

class demultiplexer
{
public:
    virtual ~demultiplexer() = default;
    virtual bool init() = 0;
    virtual int  add_event(event* ev, int events) = 0;
    virtual void del_event(event* ev) = 0;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) = 0;
    virtual void wakeup() = 0;

    auto& get_wakeup() { return _wakeup; }

protected:
#ifdef _REENTRANT
    std::atomic<bool> _wakeup = false;
#else
    bool _wakeup = false;
#endif
    int _wakeup_fd = -1;
};

class epoller : public demultiplexer
{
public:
    virtual ~epoller() override;
    virtual bool init() override;
    virtual int  add_event(event* ev, int events) override;
    virtual void del_event(event* ev) override;
    virtual void dispatch(reactor* base, int timeout/*ms*/ = 1000) override;
    virtual void wakeup() override;

private:
    int _epfd = -1;
    std::set<int> _listenfds;
};


} // namespace bee