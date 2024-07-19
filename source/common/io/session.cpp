#include "session.h"

#include "address.h"

class session_manager;

session::session(session_manager* manager)
    : _manager(manager)
{
    
}

void session::open(address* peer)
{
    _state = SESSION_STATE_ACTIVE;
    _peer = peer->dup();
}

void session::close()
{

    delete _peer;
    _state = SESSION_STATE_CLOSING;
}