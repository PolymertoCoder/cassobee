#pragma once
#include "address.h"
#include "octets.h"
#include "session_manager.h"

class session
{
public:
    session();
    void open();
    void close();

protected:
    SID _sid = 0;
    int _sockfd = 0;
    address* _peer = nullptr;
    session_manager _manager;

    int _rlength = 0;
    octets _readbuf;
    int _wlength = 0;
    octets _writebuf;
};