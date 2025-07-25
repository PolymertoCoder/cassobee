#include <cerrno>
#include <openssl/bio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>
#include <openssl/err.h>

#include "ioevent.h"
#include "glog.h"
#include "address.h"
#include "event.h"
#include "common.h"
#include "reactor.h"
#include "session.h"
#include "session_manager.h"

namespace bee
{

bool io_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        handle_read();
    }
    if(active_events & EVENT_SEND)
    {
        handle_write();
    }
    handle_close();
    return true;
}

stdio_event::stdio_event(int fd, size_t buffer_size)
{
    _fd = fd;
    _buffer.reserve(buffer_size);
    set_nonblocking(_fd);
}

bool stdio_event::handle_event(int active_events)
{
    if(active_events & EVENT_RECV)
    {
        size_t n = handle_read();
        return on_read(n) || on_error(n);
    }
    if(active_events & EVENT_SEND)
    {
        size_t n = handle_write();
        return on_write(n);
    }
    return false;
}

stdin_event::stdin_event(size_t buffer_size)
    : stdio_event(fileno(stdin), buffer_size)
{
    set_events(EVENT_RECV);
}

int stdin_event::handle_read()
{
    size_t total_read = 0;
    while(true)
    {
        size_t free_size = _buffer.free_space();
        if(free_size == 0) break;

        char* recv_ptr = _buffer.end();
        int len = read(_fd, recv_ptr, free_size);
        if(len > 0)
        {
            _buffer.fast_resize(len);
            total_read += len;
            local_log("stdio_event handle_event read %d bytes from stdin.", len);
        }
        else if(len == 0)
        {
            local_log("stdio_event handle_event stdin closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                local_log("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
                return true;
            }
            else
            {
                perror("read");
                return false;
            }
        }
    }
    return total_read;
}

stdout_event::stdout_event(size_t buffer_size)
    : stdio_event(fileno(stdout), buffer_size)
{
    set_events(EVENT_SEND);
}

int stdout_event::handle_write()
{
    size_t total_write = 0;
    while(true)
    {
        size_t free_size = _buffer.free_space();
        if(free_size == 0) break;

        char* recv_ptr = _buffer.end();
        int len = write(_fd, recv_ptr, free_size);
        if(len > 0)
        {
            total_write += len;
            local_log("stdio_event handle_event write %d bytes to stdout.", len);
        }
        else if(len == 0)
        {
            local_log("stdio_event handle_event stdout closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                local_log("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
                return true;
            }
            else
            {
                perror("write");
                return false;
            }
        }
    }
    return total_write;
}

stderr_event::stderr_event(size_t buffer_size)
    : stdio_event(fileno(stderr), buffer_size)
{
    set_events(EVENT_RECV);
}

int stderr_event::handle_read()
{
    size_t total_read = 0;
    while(true)
    {
        size_t free_size = _buffer.free_space();
        if(free_size == 0) break;

        char* recv_ptr = _buffer.end();
        int len = read(_fd, recv_ptr, free_size);
        if(len > 0)
        {
            _buffer.fast_resize(len);
            total_read += len;
            local_log("stdio_event handle_event read %d bytes from stdin.", len);
        }
        else if(len == 0)
        {
            local_log("stdio_event handle_event stdin closed.");
            return false;
        }
        else
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                local_log("stdio_event handle_event temporary error (errno=%d: %s).", errno, strerror(errno));
                return true;
            }
            else
            {
                perror("read");
                return false;
            }
        }
    }
    return total_read;
}

netio_event::netio_event(session* ses) : _ses(ses)
{
    _ses->set_event(this);
}

netio_event::~netio_event()
{
    if(_fd >= 0)
    {
        ::close(_fd);
        _fd = -1;
    }
    if(_ses)
    {
        _ses->close();
        delete _ses;
        _ses = nullptr;
    }
}

void netio_event::handle_close()
{
    if(_ses && _ses->is_close())
    {
        if(!_ses->is_writeos_empty())
        {
            _ses->permit_send(); // 断开连接前把数据发完
        }
        else
        {
            del_event();
        }
    }
}

void netio_event::close_socket(int reason) // 强制断开
{
    _ses->set_close(reason);
    del_event();
}

passiveio_event::passiveio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    set_events(EVENT_ACCEPT);
    int socktype = _ses->get_manager()->socktype();
    
    if(socktype == SOCK_STREAM)
    {
        int family = manager->family();
        int listenfd = socket(family, socktype, 0);
        int opt = 1;
        if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            close(listenfd);
            exit(EXIT_FAILURE);
        }    
        set_nonblocking(listenfd);

        struct sockaddr* addr = manager->get_addr()->addr();
        if(bind(listenfd, addr, sizeof(*addr)) < 0)
        {
            perror("bind");
            close(listenfd);
            local_log("passiveio_event bind failed, addr:%s.", manager->get_addr()->to_string().data());
            return;
        }
        if(listen(listenfd, 20) < 0)
        {
            perror("listen");
            close(listenfd);
            local_log("passiveio_event listen failed, addr:%s.", manager->get_addr()->to_string().data());
            return;
        }
        _fd = listenfd;
        local_log("fd %d listen addr:%s.", listenfd, manager->get_addr()->to_string().data());
    }
    else if(socktype == SOCK_DGRAM)
    {
        // TODO
    }
}

bool passiveio_event::handle_event(int active_events)
{
    if(_base == nullptr) return false;
    if(active_events != EVENT_ACCEPT) return false;

    struct sockaddr_storage sock_client;
    socklen_t len = sizeof(sock_client);
    int clientfd = accept(_fd, (struct sockaddr*)&sock_client, &len);
    if(clientfd < 0)
    {
        if(errno != EAGAIN && errno != EINTR)
        {
            local_log("accept: %s", strerror(errno));
            return false;
        }
        return true;
    }
    if(set_nonblocking(clientfd) < 0)
    {
        close(clientfd);
        return false;
    }

    streamio_event* evt = new streamio_event(clientfd, _ses->dup());
    evt->set_events(EVENT_RECV | EVENT_SEND);
    _base->add_event(evt);
    local_log("accept clientid=%d.", clientfd);
    return true;
}

activeio_event::activeio_event(session_manager* manager)
    : netio_event(manager->create_session())
{
    set_events(EVENT_SEND);
    _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_fd < 0)
    {
        perror("create socket");
        return;
    }
    set_nonblocking(_fd);
}

bool activeio_event::handle_event(int active_events)
{
    if(_base == nullptr) return false;

    struct sockaddr* addr = _ses->get_manager()->get_addr()->addr();
    while(connect(_fd, addr, sizeof(*addr)) < 0)
    {
        if(errno == EINTR) continue;
        if(errno != EINPROGRESS)
        {
            perror("connect");
            close_socket(SESSION_CLOSE_REASON_ERROR);
            return false;
        }
    }

    int optval = 0; socklen_t optlen = sizeof(optval);
    if(getsockopt(_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0 || optval != 0)
    {
        if(optval != 0)
        {
            errno = optval;
            perror("connect");
        }
        else
        {
            perror("getsockopt");
        }
        close_socket(SESSION_CLOSE_REASON_ERROR);
        return false;
    }

    // 所有权转移给新的streamio_event
    streamio_event* evt = new streamio_event(_fd, _ses);
    evt->set_events(EVENT_RECV | EVENT_SEND | EVENT_HUP);
    evt->set_status(EVENT_STATUS_ADD);
    _base->add_event(evt);
    local_log("activeio_event handle_event run fd=%d.", _fd);
    _fd = -1;
    _ses = nullptr;
    return true;
}

streamio_event::streamio_event(int fd, session* ses)
    : netio_event(ses)
{
    _fd = fd;
    address* addr = ses->get_peer();
    if(getsockname(_fd, addr->addr(), &addr->len()) < 0)
    {
        perror("getsockname");
        close_socket(SESSION_CLOSE_REASON_ERROR);
        return;
    }
    int flag = 1;
    if(setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int)) < 0)
    {
        perror("setsockopt");
        close_socket(SESSION_CLOSE_REASON_ERROR);
        return;
    }
    if(setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
    {
        perror("setsockopt");
        close_socket(SESSION_CLOSE_REASON_ERROR);
        return;
    }
    ses->set_open();
    //local_log("streamio_event constructor run");
}

int streamio_event::handle_read()
{
    octets& rbuffer = _ses->rbuffer();
    size_t free_space = rbuffer.free_space();
    if(free_space == 0)
    {
        _ses->on_recv(rbuffer.size());
        local_log("recv[fd=%d]: session %lu buffer is fulled, no space to receive data", _fd, _ses->get_sid());
    }
    
    int total_recv = 0;
    while(true)
    {
        size_t free_space = rbuffer.free_space();
        if(free_space == 0)
        {
            local_log("recv[fd=%d]: session %lu buffer is fulled on receiving", _fd, _ses->get_sid());
            break;
        }

        char* recv_ptr = rbuffer.end();
        int len = recv(_fd, recv_ptr, free_space, MSG_NOSIGNAL);
        if(len > 0)
        {
            rbuffer.fast_resize(len);
            total_recv += len;
            local_log("recv[fd=%d]: received %d bytes, buffer size=%zu, free space=%zu",
                _fd, len, rbuffer.size(), rbuffer.free_space());
        }
        else if(len == 0)
        {
            local_log("recv[fd=%d]: connection closed by peer, buffer size=%zu",
                _fd, rbuffer.size());
            close_socket(SESSION_CLOSE_REASON_REMOTE);
            break;
        }
        else // len < 0
        {
            if(errno == EINTR) continue; // 被信号中断，继续接收
            if(errno == EAGAIN) break; // 非阻塞模式下没有数据可读
            perror("recv");
            local_log("recv[fd=%d]: error (errno=%d: %s), closing socket, buffer size=%zu",
                _fd, errno, strerror(errno), rbuffer.size());
            close_socket(SESSION_CLOSE_REASON_ERROR);
            break;
        }
    }
    
    _ses->on_recv(total_recv);
    return total_recv;
}

int streamio_event::handle_write()
{
    octets* wbuffer = nullptr;
    {
        bee::rwlock::wrscoped sesl(_ses->_locker);
        wbuffer = &_ses->wbuffer();
        if(wbuffer->size() == 0)
        {
            _ses->forbid_send();
            return 0;
        }
    }

    int total_send = 0;
    while(true)
    {
        int len = send(_fd, wbuffer->data() + _ses->_write_offset, wbuffer->size() - _ses->_write_offset, MSG_NOSIGNAL);
        if(len > 0)
        {
            //local_log("send data:%s", std::string(wbuffer.peek(0), len).data());
            total_send += len;
            _ses->_write_offset += len;
        }
        else if(len == 0)
        {
            local_log("Send returned 0: Connection might be closed by the peer or is otherwise unusable.");
            close_socket(SESSION_CLOSE_REASON_ERROR);
            break;
        }
        else // len < 0
        {
            if(errno == EINTR) continue;
            if(errno == EAGAIN) break;
            perror("send");
            close_socket(SESSION_CLOSE_REASON_ERROR);
            break;
        }

        if(_ses->_write_offset == wbuffer->size())
        {
            _ses->clear_wbuffer();
            bee::rwlock::wrscoped sesl(_ses->_locker);
            wbuffer = &_ses->wbuffer();
            if(wbuffer->size() == _ses->_write_offset)
            {
                _ses->forbid_send();
                break;
            }
        }
    }

    _ses->on_send(total_send);
    return total_send;
}

} // namespace bee