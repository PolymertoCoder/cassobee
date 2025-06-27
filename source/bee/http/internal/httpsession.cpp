#include "httpsession.h"
#include "address.h"
#include "glog.h"
#include "httpprotocol.h"
#include "log.h"
#include "httpsession_manager.h"
#include "session.h"
#include "systemtime.h"

namespace bee
{

httpsession::httpsession(httpsession_manager* manager)
    : session(manager)
{
}

httpsession::~httpsession()
{
    delete _unfinished_protocol;
}

httpsession* httpsession::dup()
{
    auto ses = new httpsession(*this);
    ses->_sid = 0;
    ses->_sockfd = 0;
    ses->_state = SESSION_STATE_NONE;
    ses->_peer = _peer->dup();

    ses->_event = nullptr;
    ses->_readbuf.clear();
    ses->_writebuf.clear();
    ses->_reados.clear();
    ses->_writeos.clear();
    ses->_requests = 0;
    ses->_unfinished_protocol = nullptr;
    return ses;
}

void httpsession::on_recv(size_t len)
{
    activate();

    local_log("httpsession::on_recv len=%zu, _readbuf size:%zu _reados size:%zu.", len, _readbuf.size(), _reados.size());
    set_state(SESSION_STATE_RECVING);

    size_t append_length = std::min(_readbuf.size(), _reados.data().free_space());
    _reados.data().append(_readbuf, append_length);
    _readbuf.erase(0, append_length);
    local_log("on_recv append size:%zu, _readbuf size:%zu _reados size:%zu.", append_length, _readbuf.size(), _reados.size());

    while(httpprotocol* prot = httpprotocol::decode(_reados, this))
    {
        static_cast<httpsession_manager*>(_manager)->handle_protocol(prot);
    }
    _reados.try_shrink();
}

void httpsession::on_send(size_t len)
{
    activate();
    session::on_send(len);
}

} // namespace bee