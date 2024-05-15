#pragma once
#include "types.h"
#include "reactor.h"

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

struct netio_event : event
{
    netio_event(int fd) : event(fd) {}
    SID _sid;
    int _rlength;
    char _readbuf[READ_BUFFER_SIZE];
    int _wlength;
    char _writebuf[WRITE_BUFFER_SIZE];
};

struct streamio_event : netio_event
{
    streamio_event(int fd) : netio_event(fd) {}
    virtual bool handle_event(int active_events) override;
    int handle_accept();
    int handle_recv();
    int handle_send();
};

struct timer_event : event
{
    using callback = bool(*)(void* param);
    timer_event(int repeats, int timeout, callback);
    virtual int get_handle() override { return _timeout; }
    virtual bool handle_event(int active_events) override;
    int _repeats;
    int _timeout;
    callback _callback;
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
