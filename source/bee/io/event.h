#pragma once
#include <functional>

#include "types.h"

namespace bee
{

class reactor;

enum EVENT_MASK
{
    EVENT_NONE   = 0x00,
    EVENT_ACCEPT = 0x01,
    EVENT_RECV   = 0x02,
    EVENT_SEND   = 0x04,
    EVENT_HUP    = 0x08,
    EVENT_TIMER  = 0x10,
    EVENT_SIGIO  = 0x20,
    EVENT_SIGNAL = 0x40,
    EVENT_WAKEUP = 0x80,
};

enum EVENT_STATUS
{
    EVENT_STATUS_NONE = 0,
    EVENT_STATUS_ADD = 1,
    EVENT_STATUS_DEL = 2,
};

struct event
{
    event() : _status(EVENT_STATUS_NONE) {}
    virtual ~event()
    {
        _status = EVENT_STATUS_NONE;
        _events = EVENT_NONE;
        _base = nullptr;
    }
    virtual int get_handle() const { return 0; }
    virtual bool handle_event(int active_events) { return 0; };
    int  get_status() const { return _status; }
    void set_status(int status) { _status = status; }
    int  get_events() const { return _events; }
    void set_events(int events) { _events = events; }
    void del_event();
    bool is_close() const { return _status == EVENT_STATUS_DEL; }

    int _status = EVENT_STATUS_NONE;
    int _events = EVENT_NONE;
    reactor* _base = nullptr;
};

struct control_event : event
{
    control_event();
    ~control_event();
    virtual int get_handle() const override { return _efd; }
    virtual bool handle_event(int active_events) override;
    int _efd = -1;
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
    void* _param = nullptr;
};

struct sigio_event : event
{
    using signal_handler = void(*)(int);
    sigio_event();
    virtual int get_handle() const override { return _signal_pipe[0]; }
    virtual bool handle_event(int active_events) override;
    static void sigio_callback(int signum);
    static int _signal_pipe[2];
};

struct signal_event : event
{
    using signal_callback = bool(*)(int);
    signal_event(int signum, signal_callback callback);
    virtual int get_handle() const override { return _signum; }
    virtual bool handle_event(int active_events) override;
    int _signum;
    signal_callback _callback;
};

} // namespace bee