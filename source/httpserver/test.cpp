#include "reactor.h"
#include "event.h"
#include "demultiplexer.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 7710
#define SERVER_NUM 1

int main(int argc,char *argv[]){

    unsigned short port = SERVER_PORT;

    if(argc ==2)
        port  = atoi(argv[1]);

    epoller* epollde = new epoller();
    reactor *m_reactor = new reactor(epollde);

    if(NULL == m_reactor)
        return 0;

    int i =0;
    int lisfd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(lisfd,F_SETFL,O_NONBLOCK);

    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sock));

    sock.sin_addr.s_addr =htonl(INADDR_ANY);
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port+i); 

    bind(lisfd,(struct sockaddr*)&sock,sizeof(sock));

    listen(lisfd,20);
    event* evHanlde =  new common_event(lisfd);
    m_reactor->add_event(evHanlde,EVENT_ACCEPT);
    m_reactor->run();
    close(lisfd);
    delete m_reactor;

    return 0;
}
