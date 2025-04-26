#pragma once
#include "ioevent.h"

namespace bee
{

struct ssl_passiveio_event : netio_event
{
    ssl_passiveio_event(session_manager* manager);
    virtual bool handle_event(int active_events) override;
};

struct ssl_activeio_event : netio_event
{
    ssl_activeio_event(session_manager* manager);
    virtual bool handle_event(int active_events) override;
};

struct sslio_event : netio_event
{
    sslio_event(int fd, SSL_CTX* ssl_ctx, bool is_server, session* ses);
    virtual ~sslio_event() override;
    virtual bool handle_event(int active_events) override;
    bool handle_handshake();
    int  handle_recv();
    int  handle_send();
    void cleanup_ssl();
    
    SSL_CTX* _ssl_ctx = nullptr; // session_manager::_ssl_ctx的引用
    SSL* _ssl = nullptr;
    bool _is_server = false;
    bool _handshake_done = false;
    TIMETYPE _handshake_starttime = 0;
};

} // namespace bee