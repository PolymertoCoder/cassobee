#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
using namespace std;

int main()
{
    int listen_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_sockfd < 0)
    {
        perror("socket");
        return 0;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(45678);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int ret = bind(listen_sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        perror("bind");
        return 0;
    }

    ret = listen(listen_sockfd, 5);
    if(ret < 0)
    {
        perror("listen");
        return 0;
    }

    int epoll_fd = epoll_create(1);
    if(epoll_fd < 0)
    {
        perror("epoll_create");
        return 0;
    }

    struct epoll_event ee;
    ee.events = EPOLLIN;
    ee.data.fd = listen_sockfd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sockfd, &ee);

    struct epoll_event arr[10];
    while(1)
    {
        int ret = epoll_wait(epoll_fd, arr, 10, -1);
        if(ret < 0)
        {
            perror("epoll_wait");
            continue;
        }
        else if(ret == 0)
        {
            continue;
        }

        for(int i = 0; i < ret; i++)
        {
            if(arr[i].data.fd == listen_sockfd)
            {
                // 侦听到了, 调用accept
                int newsockfd = accept(listen_sockfd, NULL, NULL);
                if(newsockfd < 0)
                {
                    perror("accept");
                    i--;
                    continue;
                }

                int flag = fcntl(newsockfd, F_GETFL);
                fcntl(newsockfd, F_SETFL, flag | O_NONBLOCK);

                struct epoll_event new_ee;
                new_ee.events = EPOLLIN | EPOLLET;
                new_ee.data.fd = newsockfd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &new_ee);
            }
            else
            {
                // recv
                int newsockfd = arr[i].data.fd;
                string recv_buf;
                bool flag = true;
                while(1)
                {
                    char buf[2] = {0};
                    ssize_t recv_size = recv(newsockfd, buf, sizeof(buf), 0);
                    if(recv_size < 0)
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // 接受缓冲区数据已读取完毕
                            flag = true;
                            break; 
                        }
                        flag = false;
                        perror("recv");
                        continue;
                    }
                    else if(recv_size == 0)
                    {
                        printf("peer shutdown\n");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, newsockfd, NULL);
                        close(newsockfd);
                        break;
                    }

                    printf("%s\n", buf);

                    recv_buf += buf;
                }
                if(flag)
                {
                    printf("recv_buf: %s\n", recv_buf.c_str());

                    char buf[1024] = {0};
                    memset(buf, '\0', sizeof(buf));
                    strncpy(buf, "hello, i am server~~", sizeof(buf));
                    send(newsockfd, buf, strlen(buf), 0);

                }
            }
        }

        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listen_sockfd, NULL);
        close(listen_sockfd);

        return 0;
    }
}
