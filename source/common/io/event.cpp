#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include "reactor.h"
#include "event.h"
#include "log.h"

int control_event::_pipe[2] = {-1, -1};

control_event::control_event()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, control_event::_pipe) == -1)
    {
        local_log("sigio_event create failed.");
        return;
    }
}

bool control_event::handle_event(int active_events)
{
    char buf[32];
    size_t n = read(_pipe[0], buf, 32);
    if(n == 0 || n > 32 || !_base) return false;
    return true;
}

void control_event::wakeup(reactor* base)
{
    if(!base || base->get_wakeup()) return;
    write(_pipe[1], "0", 1);
    base->get_wakeup() = false;
}

timer_event::timer_event(bool delay, TIMETYPE timeout, int repeats, callback handler, void* param)
    : _delay(delay), _timeout(timeout), _repeats(repeats), _handler(handler), _param(param)
{
}

bool timer_event::handle_event(int active_events)
{
    if(_handler(_param))
    {
        if(_repeats > 0){ --_repeats; }
        if(_repeats != 0) return true;
    }
    return false;
}

int sigio_event::_pipe[2] = { -1, -1 };

sigio_event::sigio_event()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, sigio_event::_pipe) == -1)
    {
        local_log("sigio_event create failed.");
        return;
    }
}

bool sigio_event::handle_event(int active_events)
{
    char buf[32];
    size_t n = read(sigio_event::_pipe[0], buf, 32);
    if(n == 0 || n > 32 || !_base) return false;
    for(size_t i = 0; i < n; ++i){ if(!_base->handle_signal_event(i)) return false; }
    return true;
}

void sigio_event::sigio_callback(int signum)
{
    write(sigio_event::_pipe[1], (char*)&signum, 1);
}

signal_event::signal_event(int signum, signal_callback callback)
    : _signum(signum), _callback(callback)
{
    set_signal(signum, sigio_event::sigio_callback);
}

bool signal_event::handle_event(int active_events)
{
    if(active_events != EVENT_SIGNAL) return false;
    if(!_callback(_signum)) return false;
    return true;
}