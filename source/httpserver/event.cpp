#include <memory.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../util.h"
#include "event.h"

#define BUFFER_LEN_MAX 1024

void common_event::handle_accept(void* param)
{
    reactor* base = (reactor*)param;
    if(base == nullptr) return;

    struct sockaddr_in sock_client;
    socklen_t len = sizeof(sock_client);
    int clientfd = accept(_fd, (sockaddr*)&sock_client, &len);
    if(clientfd == -1) return;

    int ret = set_nonblocking(clientfd, true);
    if(ret < 0) return;

    event* ev = new common_event(clientfd);
    if(ev == nullptr) return;
    base->add_event(ev, EVENT_RECV);
}

void common_event::handle_recv(void* param)
{
    reactor* base = (reactor*)param;
    if(base == nullptr) return;
    
    event* ev = base->get_event(_fd);
    if(ev == nullptr) return;

    unsigned char buffer[BUFFER_LEN_MAX];
    memset(buffer, 0, BUFFER_LEN_MAX);

    int idx = 0, len = 0;
    while(idx < BUFFER_LEN_MAX)
    {
        len = recv(_fd, buffer, BUFFER_LEN_MAX, 0);

        if(len > 0) {

        }
        else if(len == 0) {
            close(_fd);
        }
        else {
            close(_fd);
        }
    }
}

void common_event::handle_send(void* param)
{
    reactor* base = (reactor*)param;
    if(base == nullptr) return;
}
