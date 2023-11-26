#pragma once
#include "reactor.h"
#include "event.h"

struct event;
class reactor;

class demultiplexer
{
public: 
    demultiplexer(){}
    virtual ~demultiplexer(){}
    virtual int add_event(event* ev, int events){ return 0; };
    virtual void del_event(event* ev){};
    virtual void dispatch(reactor* base, int timeout = 1000){};
};

class epoller : public demultiplexer
{
public:
    epoller(){};
    virtual ~epoller(){};
    virtual int add_event(event* ev, int events) override;
    virtual void del_event(event* ev) override;
    virtual void dispatch(reactor* base, int timeout = 1000) override;

private:
    int _epfd;
    int _listenfd;
};

