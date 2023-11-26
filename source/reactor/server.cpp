#include "reactor.h"
#include "event.h"
#include "demultiplexer.h"
#include "block_list.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>

#define SERVER_PORT 8888

int main(int argc,char *argv[])
{
#if 1
    unsigned short port = SERVER_PORT;
    int listenfd_count = 1;

    if(argc == 2)
        listenfd_count = atoi(argv[1]);

    epoller* epollde = new epoller();
    epollde->init();
    reactor *m_reactor = new reactor(epollde);

    if(NULL == m_reactor)
        return 0;

    std::vector<int> lisfds;
    for(int i = 0; i < listenfd_count; ++i)
    {
        int lisfd = socket(AF_INET, SOCK_STREAM, 0);
        lisfds.push_back(lisfd);
        fcntl(lisfd, F_SETFL, O_NONBLOCK);

        struct sockaddr_in sock;
        memset(&sock, 0, sizeof(sock));

        sock.sin_addr.s_addr =htonl(INADDR_ANY);
        sock.sin_family = AF_INET;
        sock.sin_port = htons(port+i); 

        bind(lisfd, (struct sockaddr*)&sock, sizeof(sock));

        listen(lisfd,20);
        event* evHanlde = new common_event(lisfd);
        int ret = m_reactor->add_event(evHanlde,EVENT_ACCEPT);
        printf("listenfd %d add accept event, ret=%d, port=%d.\n", lisfd, ret, port+i);
    }
    m_reactor->run();
    
    for(int i = 0; i < 100; ++i)
    {
        close(lisfds[i]);
    }
    delete m_reactor;

#elif 0
    {
        GET_TIME_BEGIN();
        block_list<int> list;
        for(size_t i = 0; i < 100000; ++i)
        {
            list.insert(i, i * 100);
        }

        for(size_t i = 0; i < 10000000; ++i)
        {
            int key = rand(0, 100000);
            list[key] += 2000;
        }
        GET_TIME_END();
    }
    {
        GET_TIME_BEGIN();
        std::map<int, int> list;
        for(size_t i = 0; i < 100000; ++i)
        {
            list.emplace(i, i * 100);
        }

        for(size_t i = 0; i < 10000000; ++i)
        {
            int key = rand(0, 100000);
            list[key] += 2000;
        }
        GET_TIME_END();
    }
    {
        GET_TIME_BEGIN();
        std::multimap<int, int> list;
        for(size_t i = 0; i < 100000; ++i)
        {
            list.emplace(i, i * 100);
        }

        for(size_t i = 0; i < 10000000; ++i)
        {
            int key = rand(0, 100000);
            list.find(key)->second += 2000;
        }
        GET_TIME_END();
    }

#endif

    return 0;
}
