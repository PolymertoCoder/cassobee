#include "session.h"

#include "ExampleProtocol.h"
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
}

session::~session()
{
    delete _peer;
}

session* session::dup()
{
    auto ses = new session(*this);
    ses->_sid = 0;
    ses->_sockfd = 0;
    ses->_state = SESSION_STATE_NONE;
    ses->_peer->dup();

    ses->_event = nullptr;
    _readbuf.clear();
    _writebuf.clear();
    _reados.clear();
    _writeos.clear();
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
}

#include "ExampleProtocol.h"
void session::on_recv(size_t len)
{
    TRACELOG("session::on_recv len=%zu.", len);
    set_state(SESSION_STATE_RECVING);
    _reados << _readbuf;

    std::string content;
    _reados >> content;
    TRACELOG("receive client data:%s.", content.data());
    _manager->send_protocol(_sid, ExampleProtocol());

    // while(protocol* prot = protocol::decode(_reados, this))
    // {
    //     threadpool::get_instance()->add_task(prot->thread_group_idx(), [prot]()
    //     {
    //         prot->run();
    //         delete prot;
    //     });
    // }
}

void session::on_send(size_t len)
{
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
    reactor::get_instance()->add_event(_event, EVENT_RECV);
}

void session::permit_send()
{
    reactor::get_instance()->add_event(_event, EVENT_SEND);
}