#pragma once
#include "event.h"
#include "httpsession.h"
#include <openssl/ssl.h>

namespace bee
{

class session;
class httpsession;
class session_manager;

struct io_event : event
{
    io_event() {}
    virtual int get_handle() const override { return _fd; }
    int _fd = -1;
};

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
    virtual int handle_recv();
    virtual int handle_send();
};

class sslio_event : public streamio_event
{
public:
    sslio_event(int fd, httpsession* ses);
    virtual ~sslio_event();

    virtual bool handle_event(int active_events) override;
    
protected:
    bool do_handshake();
    virtual int handle_recv() override;
    virtual int handle_send() override;

private:
    SSL* _ssl;
    enum class SSLState {
        HANDSHAKE,
        STREAMING,
        ERROR
    } _ssl_state = SSLState::HANDSHAKE;
};

} // namespace bee