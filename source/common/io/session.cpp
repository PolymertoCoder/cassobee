#include "session.h"

#include "address.h"
#include "event.h"
#include "protocol.h"
#include "reactor.h"

class session_manager;

session::session(session_manager* manager)
    : _manager(manager)
{
    _readbuf.reserve(_manager->_read_buffer_size);
    _writebuf.reserve(_manager->_write_buffer_size);
}

void session::on_recv(size_t len)
{
    _state = SESSION_STATE_RECVING;
    _reados << _readbuf;

    protocol::decode(_reados);
}

void session::on_send(size_t len)
{
    if(len == _writebuf.size())
    {
        // 写缓冲区的数据全部发送完毕了
        permit_recv();
    }
    else
    {
        // 写缓冲区的数据还有剩余，需要继续等待写事件触发
        permit_send();
    }
}

void session::permit_recv()
{
    reactor::get_instance()->add_event(_event, EVENT_RECV);
}

void session::permit_send()
{
    reactor::get_instance()->add_event(_event, EVENT_SEND);
}

void session::open(address* peer)
{
    set_state(SESSION_STATE_ACTIVE);
    _peer = peer->dup();
}

void session::close()
{

    delete _peer;
    set_state(SESSION_STATE_CLOSING);
}