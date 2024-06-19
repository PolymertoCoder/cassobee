#include "session_manager.h"
#include "address.h"
#include "common.h"
#include "event.h"
#include "ioevent.h"
#include "reactor.h"
#include <cstring>

session_manager::session_manager()
{

}

void client(session_manager* manager)
{
    
}

void server(session_manager* manager)
{
    address* local = manager->_local;
    switch(manager->_socktype)
    {
        case AF_INET:
        {
            ipv4_address* localaddr = dynamic_cast<ipv4_address*>(local);
            int listenfd = socket(AF_INET, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in sock;
            bzero(&sock, sizeof(sock));
            sock.sin_addr.s_addr = htonl(INADDR_ANY);
            sock.sin_family = AF_INET;
            sock.sin_port = htons(localaddr->_addr.sin_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
        case AF_INET6:
        {
            int listenfd = socket(AF_INET6, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            address* local = manager->_local->dup();
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
    }
}