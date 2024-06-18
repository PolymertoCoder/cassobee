#pragma once
#include "address.h"
#include "octets.h"
#include "event.h"
#include "marshal.h"
#include "session_manager.h"

enum SESSION_STATE
{
    SESSION_STATE_NONE,
    SESSION_STATE_ACTIVE,
    SESSION_STATE_SENDING,
    SESSION_STATE_RECVING,
    SESSION_STATE_CLOSING
};

class session
{
public:
    session(session_manager* manager);
    void open(address* peer);
    void close();

public:
    SID _sid = 0;
    int _sockfd = 0;
    uint8_t _state = SESSION_STATE_NONE;
    address* _peer = nullptr;
    session_manager* _manager;

    event* _event;
    int _rlength = 0;
    octets _readbuf;
    int _wlength = 0;
    octets _writebuf;

    octetsstream _reados;
    octetsstream _writeos;
};