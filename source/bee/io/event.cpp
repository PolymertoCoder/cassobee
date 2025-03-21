#include <cstdio>
#include <print>
#include <unistd.h>
#include <sys/socket.h>
#include "demultiplexer.h"
#include "reactor.h"
#include "event.h"
#include "types.h"

namespace bee
{

control_event::control_event()
{
    set_events(EVENT_WAKEUP);
    if(pipe(_control_pipe) == -1)
    {
        perror("pipe");
        return;
    }
    set_nonblocking(_control_pipe[0]);
    set_nonblocking(_control_pipe[1]);
}

control_event::~control_event()
{
    close(_control_pipe[0]);
    close(_control_pipe[1]);
}

bool control_event::handle_event(int active_events)
{
    char data;
    if(int n = read(_control_pipe[0], &data, sizeof(data)); n > 0)
    {
        CHECK_BUG(data == 0, return false);
    }
    std::println("control_event wakeup...");
    return true;
}

timer_event::timer_event(bool delay, TIMETYPE timeout, int repeats, callback handler, void* param)
    : _delay(delay), _timeout(timeout), _repeats(repeats), _handler(handler), _param(param)
{
    set_events(EVENT_TIMER);
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

int sigio_event::_signal_pipe[2] = { -1, -1 };

sigio_event::sigio_event()
{
    set_events(EVENT_RECV);
    if(pipe(_signal_pipe) == -1)
    {
        perror("pipe");
        return;
    }
    set_nonblocking(_signal_pipe[0]);
    set_nonblocking(_signal_pipe[1]);
    std::println("sigio_event readfd=%d writefd=%d.", _signal_pipe[0], _signal_pipe[1]);
}

bool sigio_event::handle_event(int active_events)
{
    std::println("sigio_event handle_event run.");
    if(!_base) return false;
    int signum;
    if(read(_signal_pipe[0], &signum, sizeof(signum)) == -1)
    {
        perror("read");
        return false;
    }
    if(!_base->handle_signal_event(signum)) return false;
    std::println("sigio_event handle_event run success, signum=%d.", signum);
    return true;
}

void sigio_event::sigio_callback(int signum)
{
    write(_signal_pipe[1], &signum, sizeof(signum));
    std::println("sigio_event callback run.");
}

signal_event::signal_event(int signum, signal_callback callback)
    : _signum(signum), _callback(callback)
{
    std::println("signal_event constructor, signum=%d.", _signum);
    set_signal(signum, sigio_event::sigio_callback);
}

bool signal_event::handle_event(int active_events)
{
    std::println("signal_event handle_event run, _signum=%d.", _signum);
    if(active_events != EVENT_SIGNAL) return false;
    if(!_callback(_signum)) return false;
    return true;
}

} // namespace bee