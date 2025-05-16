#pragma once
#include "event.h"
#include "octets.h"
#include <cstdio>

namespace bee
{

class session;
class session_manager;

struct io_event : event
{
    virtual int get_handle() const override { return _fd; }
    int _fd = -1;
};

// 标准输入/输出/错误
struct stdio_event : io_event
{
    stdio_event(int fd, size_t buffer_size);
    size_t on_read();
    size_t on_write();
    octets _buffer;
};

struct stdin_event : stdio_event
{
    stdin_event(size_t buffer_size);
    virtual bool handle_event(int active_events) override;
    virtual bool handle_read(size_t n) = 0;
};

struct stdout_event : stdio_event
{
    stdout_event(size_t buffer_size);
    virtual bool handle_event(int active_events) override;
    virtual bool handle_write(size_t n) = 0;
};

struct stderr_event : stdio_event
{
    stderr_event(size_t buffer_size);
    virtual bool handle_event(int active_events) override;
    virtual bool handle_error(size_t n) = 0;
};

// 网络io
struct netio_event : io_event
{
    netio_event(session* ses);
    virtual ~netio_event();
    void close_socket();
    session* _ses;
};

struct passiveio_event : netio_event
{
    passiveio_event(session_manager* manager);
    virtual bool handle_event(int active_events) override;
};

struct activeio_event : netio_event
{
    activeio_event(session_manager* manager);
    virtual bool handle_event(int active_events) override;
};

struct streamio_event : netio_event
{
    streamio_event(int fd, session* ses);
    virtual bool handle_event(int active_events) override;
    int handle_recv();
    int handle_send();
};

} // namespace bee