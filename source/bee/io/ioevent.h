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
    virtual bool handle_event(int active_events) override;
    virtual int  handle_read() { return false; }
    virtual int  handle_write() { return false; }
    virtual void handle_close() {}
    int _fd = -1;
};

// 标准输入/输出/错误
struct stdio_event : io_event
{
    stdio_event(int fd, size_t buffer_size);
    virtual bool handle_event(int active_events) override;
    virtual bool on_read(size_t n)  { return false; };
    virtual bool on_write(size_t n) { return false; };
    virtual bool on_error(size_t n) { return false; };
    octets _buffer;
};

struct stdin_event : stdio_event
{
    stdin_event(size_t buffer_size);
    virtual int handle_read() override;
};

struct stdout_event : stdio_event
{
    stdout_event(size_t buffer_size);
    virtual int handle_write() override;
};

struct stderr_event : stdio_event
{
    stderr_event(size_t buffer_size);
    virtual int handle_read() override;
};

// 网络io
struct netio_event : io_event
{
    netio_event(session* ses);
    virtual ~netio_event();
    virtual void handle_close() override;
    void close_socket(int reason);
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
    virtual int handle_read() override;
    virtual int handle_write() override;
};

} // namespace bee