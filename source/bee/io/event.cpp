#include <cstdio>
#include <unistd.h>
#include <sys/eventfd.h>
#include "glog.h"
#include "demultiplexer.h"
#include "reactor.h"
#include "event.h"
#include "types.h"

namespace bee
{

control_event::control_event()
{
    set_events(EVENT_WAKEUP);
    _efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(_efd == -1)
    {
        perror("eventfd");
        return;
    }
}

control_event::~control_event()
{
    if(_efd >= 0)
    {
        close(_efd);
        _efd = -1;
    }
}

bool control_event::handle_event(int active_events)
{
    static eventfd_t value = 0;
    if(read(_efd, &value, sizeof(value)) < 0)
    {
        if(errno != EAGAIN)
        {
            perror("wakupfd read failed");
            return false;
        }
    }
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
    local_log("sigio_event readfd=%d writefd=%d.", _signal_pipe[0], _signal_pipe[1]);
}

bool sigio_event::handle_event(int active_events)
{
    local_log("sigio_event handle_event run.");
    if(!_base) return false;
    int signum;
    if(read(_signal_pipe[0], &signum, sizeof(signum)) == -1)
    {
        perror("read");
        return false;
    }
    if(!_base->handle_signal_event(signum)) return false;
    local_log("sigio_event handle_event run success, signum=%d.", signum);
    return true;
}

void sigio_event::sigio_callback(int signum)
{
    write(_signal_pipe[1], &signum, sizeof(signum));
    local_log("sigio_event callback run.");
}

signal_event::signal_event(int signum, signal_callback callback)
    : _signum(signum), _callback(callback)
{
    local_log("signal_event constructor, signum=%d.", _signum);
    set_signal(signum, sigio_event::sigio_callback);
}

bool signal_event::handle_event(int active_events)
{
    local_log("signal_event handle_event run, _signum=%d.", _signum);
    if(active_events != EVENT_SIGNAL) return false;
    if(!_callback(_signum)) return false;
    return true;
}

} // namespace bee