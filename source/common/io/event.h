#pragma once
#include "types.h"
#include "reactor.h"
#include <functional>

#define READ_BUFFER_SIZE 4096
#define WRITE_BUFFER_SIZE 4096

class reactor;

enum EVENT_MASK
{
    EVENT_NONE = 0,
    EVENT_ACCEPT = 0x01,
    EVENT_RECV   = 0x02,
    EVENT_SEND   = 0x04,
    EVENT_HUP    = 0x08,
    EVENT_TIMER  = 0x10,
    EVENT_SIGIO  = 0x20,
    EVENT_SIGNAL = 0x40,
};

enum EVENT_STATUS
{
    EVENT_STATUS_NONE = 0,
    EVENT_STATUS_ADD = 1,
};

struct event
{
    event() : _handle(-1), _status(EVENT_STATUS_NONE) {}
    event(int handle) : _handle(handle), _status(EVENT_STATUS_NONE) {}
    virtual ~event() {}
    virtual int get_handle() { return _handle; }
    virtual bool handle_event(int active_events) { return 0; };
    int  get_status() { return _status; }
    void set_status(int status) { _status = status; }

    int _handle;
    int _status;
    reactor* _base;
};

struct control_event : event
{
    control_event();
    virtual int get_handle() override { return _pipe[0]; }
    virtual bool handle_event(int active_events) override;
    static void wakeup(reactor* base);
    static int _pipe[2];
};

struct timer_event : event
{
    using callback = std::function<bool(void*)>;
    timer_event(bool delay, TIMETYPE timeout, int repeats, callback handler, void* param);
    virtual bool handle_event(int active_events) override;
    bool _delay;
    TIMETYPE _timeout;
    int _repeats;
    callback _handler;
    void* _param;
};

struct sigio_event : event
{
    using signal_handler = void(*)(int);
    sigio_event();
    virtual int get_handle() override { return _pipe[0]; }
    virtual bool handle_event(int active_events) override;
    static void sigio_callback(int signum);
    static int _pipe[2];
};

struct signal_event : event
{
    using signal_callback = bool(*)(int);
    signal_event(int signum, signal_callback callback);
    virtual bool handle_event(int active_events) override;
    int _signum;
    signal_callback _callback;
};