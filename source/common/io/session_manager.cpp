#include "session_manager.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "address.h"
#include "common.h"
#include "event.h"
#include "ioevent.h"
#include "reactor.h"

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
            ipv6_address* localaddr = dynamic_cast<ipv6_address*>(local);
            int listenfd = socket(AF_INET6, SOCK_STREAM, 0);
            set_nonblocking(listenfd, true);
            struct sockaddr_in6 sock;
            bzero(&sock, sizeof(sock));
            //sock.sin6_addr.s6_addr16 = htonl(INADDR_ANY);
            sock.sin6_family = AF_INET;
            sock.sin6_port = htons(localaddr->_addr.sin6_port);
            reactor::get_instance()->add_event(new passiveio_event(listenfd, local), EVENT_ACCEPT);
        } break;
    }
}