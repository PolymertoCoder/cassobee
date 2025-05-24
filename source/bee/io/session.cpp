#include "session.h"

#include "address.h"
#include "ioevent.h"
#include "protocol.h"
#include "reactor.h"
#include "systemtime.h"
#include "session_manager.h"
#ifdef _REENTRANT   
#include "threadpool.h"
#endif

namespace bee
{

session::session(session_manager* manager)
    : _manager(manager)
{
    _peer = _manager->get_addr()->dup();

    _reados.reserve(_manager->_read_buffer_size);
    _readbuf.reserve(_manager->_read_buffer_size);

    _write_offset = 0;
    _writeos.reserve(_manager->_write_buffer_size);
    _writebuf.reserve(_manager->_write_buffer_size);

    activate();
}

session::~session()
{
    delete _peer;
    delete _event;
}

void session::clear()
{
    _sid = 0;
    _sockfd = 0;
    _state = SESSION_STATE_NONE;

    _event = nullptr;

    _readbuf.clear();
    _reados.clear();

    _write_offset = 0;
    _writeos.clear();
    _writebuf.clear();
}

session* session::dup()
{
    auto ses = new session(*this);
    ses->_sid = _manager->get_next_sessionid();
    ses->_sockfd = 0;
    ses->_state = SESSION_STATE_NONE;
    ses->_peer = _peer->dup();

    ses->_event = nullptr;
    ses->_reados.clear();
    ses->_readbuf.clear();
    ses->_write_offset = 0;
    ses->_writeos.clear();
    ses->_writebuf.clear();
    return ses;
}

void session::set_open()
{
    set_state(SESSION_STATE_ACTIVE);
    _manager->add_session(_sid, this);
}

void session::set_close()
{
    set_state(SESSION_STATE_CLOSING);
}

void session::close()
{
    _manager->del_session(_sid);
    clear();
}

void session::on_recv(size_t len)
{
    activate();
    // local_log("session::on_recv len=%zu, _readbuf size:%zu _reados size:%zu.", len, _readbuf.size(), _reados.size());
    set_state(SESSION_STATE_RECVING);

    size_t append_length = std::min(_readbuf.size(), _reados.data().free_space());
    _reados.data().append(_readbuf, append_length);
    _readbuf.erase(0, append_length);
    // local_log("on_recv append size:%zu, _readbuf size:%zu _reados size:%zu.", append_length, _readbuf.size(), _reados.size());

    while(protocol* prot = protocol::decode(_reados, this))
    {
    #ifdef _REENTRANT
        threadpool::get_instance()->add_task(prot->thread_group_idx(), [prot]()
        {
            prot->run();
            delete prot;
        });
    #else
        prot->run();
        delete prot;
    #endif
    }
    _reados.try_shrink();
}

void session::on_send(size_t len)
{
    local_log("session::on_send len=%zu.", len);
    set_state(SESSION_STATE_SENDING);
}

void session::permit_recv()
{
    if(_event)
    {
        _event->permit_read();
        local_log("session %s permit_recv.", _peer->to_string().data());
    }
}

void session::permit_send()
{
    if(_event)
    {
        _event->permit_write();
        local_log("session %s permit_send.", _peer->to_string().data());
    }
}

void session::forbid_recv()
{
    if(_event)
    {
        _event->forbid_read();
        local_log("session %s forbid_recv.", _peer->to_string().data());
    }
}

void session::forbid_send()
{
    if(_event)
    {
        _event->forbid_write();
        local_log("session %s forbid_send.", _peer->to_string().data());
    }
}

size_t session::max_rbuf_size() const
{
    return _manager->_read_buffer_size; 
}

size_t session::max_wbuf_size() const
{
    return _manager->_write_buffer_size;
}

octets& session::rbuffer()
{
    return _readbuf;
}

octets& session::wbuffer()
{
    if(_writebuf.empty())
    {
        _writebuf.swap(_writeos.data());
        _writeos.clear();
    }
    return _writebuf;
}

void session::clear_wbuffer()
{
    _writebuf.clear();
    _write_offset = 0;
}

bool session::is_writeos_empty()
{
    return _write_offset == _writeos.size();
}

void session::activate()
{
    _last_active = systemtime::get_time();
}

bool session::is_timeout(TIMETYPE timeout) const
{
    return (systemtime::get_millseconds() - _last_active) > timeout;
}

} // namespace bee