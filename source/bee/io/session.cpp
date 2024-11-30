#include "session.h"

#include "address.h"
#include "event.h"
#include "log.h"
#include "protocol.h"
#include "reactor.h"
#include "threadpool.h"

session::session(session_manager* manager)
    : _manager(manager)
{
    _peer = _manager->get_addr()->dup();

    _readbuf.reserve(_manager->_read_buffer_size);
    _writebuf.reserve(_manager->_write_buffer_size);
    _reados.reserve(_manager->_read_buffer_size);
    _writeos.reserve(_manager->_write_buffer_size);
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
    _writebuf.clear();
    _reados.clear();
    _writeos.clear();
}

session* session::dup()
{
    auto ses = new session(*this);
    ses->_sid = 0;
    ses->_sockfd = 0;
    ses->_state = SESSION_STATE_NONE;
    ses->_peer = _peer->dup();

    ses->_event = nullptr;
    ses->_readbuf.clear();
    ses->_writebuf.clear();
    ses->_reados.clear();
    ses->_writeos.clear();
    return ses;
}

SID session::get_next_sessionid()
{
    static sequential_id_generator<SID, cassobee::spinlock> sid_generator;
    return sid_generator.gen();
}

void session::open()
{
    set_state(SESSION_STATE_ACTIVE);
    _sid = get_next_sessionid();
    _manager->add_session(_sid, this);
}

void session::close()
{
    set_state(SESSION_STATE_CLOSING);
    _manager->del_session(_sid);
    clear();
}

void session::on_recv(size_t len)
{
    local_log("session::on_recv len=%zu, _readbuf size:%zu _reados size:%zu.", len, _readbuf.size(), _reados.size());
    set_state(SESSION_STATE_RECVING);

    size_t append_length = std::min(_readbuf.size(), _reados.data().free_space());
    _reados.data().append(_readbuf, append_length);
    _readbuf.erase(0, append_length);
    local_log("on_recv append size:%zu, _readbuf size:%zu _reados size:%zu.", append_length, _readbuf.size(), _reados.size());

    while(protocol* prot = protocol::decode(_reados, this))
    {
        threadpool::get_instance()->add_task(prot->thread_group_idx(), [prot]()
        {
            prot->run();
            delete prot;
        });
    }
    _reados.try_shrink();
}

void session::on_send(size_t len)
{
    printf("session::on_send len=%zu.\n", len);
    set_state(SESSION_STATE_SENDING);
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
    if(!_event || _event->is_close() || (_event->get_events() & EVENT_RECV)) return;
    _event->_events |= EVENT_RECV;
    reactor::get_instance()->add_event(_event);
    local_log("session %s permit_recv.", _peer->to_string().data());
}

void session::permit_send()
{
    if(!_event || _event->is_close() || (_event->get_events() & EVENT_SEND)) return;
    _event->_events |= EVENT_SEND;
    reactor::get_instance()->add_event(_event);
    local_log("session %s permit_send.", _peer->to_string().data());
}

void session::forbid_recv()
{
    if(!_event || _event->is_close() || !(_event->get_events() & EVENT_RECV)) return;
    _event->_events &= (~EVENT_RECV);
    reactor::get_instance()->add_event(_event);
    local_log("session %s forbid_recv.", _peer->to_string().data());
}

void session::forbid_send()
{
    if(!_event || _event->is_close() || !(_event->get_events() & EVENT_SEND)) return;
    _event->_events &= (~EVENT_SEND);
    reactor::get_instance()->add_event(_event);
    local_log("session %s forbid_send.", _peer->to_string().data());
}
