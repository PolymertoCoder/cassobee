#include "session_manager.h"

#include <arpa/inet.h>
#include <cassert>
#include <netinet/in.h>
#include <sys/socket.h>

#include "address.h"
#include "common.h"
#include "event.h"
#include "ioevent.h"
#include "reactor.h"
#include "config.h"

session_manager::session_manager()
{
    auto cfg = config::get_instance();
    int version = cfg->get<int>(identity(), "version");
    if(version == 4) {
        _socktype = AF_INET;
    } else if(version == 6) {
        _socktype = AF_INET6;
    } else {
        assert(false);
    }

    _read_buffer_size = cfg->get<size_t>(identity(), "read_buffer_size");
    _write_buffer_size = cfg->get<size_t>(identity(), "write_buffer_size");
}

void client(session_manager* manager)
{
    
}

void server(session_manager* manager)
{
    address* local = manager->local();
    switch(manager->family())
    {
        case AF_INET:
        {
            ipv4_address* localaddr = dynamic_cast<ipv4_address*>(local);
            int listenfd = socket(AF_INET, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in sock;
            bzero(&sock, sizeof(sock));
            sock.sin_family = AF_INET;
            sock.sin_addr.s_addr = htonl(INADDR_ANY);
            sock.sin_port = htons(localaddr->_addr.sin_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
        case AF_INET6:
        {
            ipv6_address* localaddr = dynamic_cast<ipv6_address*>(local);
            int listenfd = socket(AF_INET6, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in6 sock;
            bzero(&sock, sizeof(sock));
            sock.sin6_family = AF_INET;
            sock.sin6_addr = in6addr_any;
            sock.sin6_port = htons(localaddr->_addr.sin6_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
    }
}