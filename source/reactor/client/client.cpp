#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
    {
        perror("socket");
        return 0;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = inet_addr("192.168.183.128");

    int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        perror("connect");
        return 0;
    }

    while(1)
    {
        char buf[1024] = "i am client abcdef";
        send(sockfd, buf, strlen(buf), 0);

        memset(buf, '\0', sizeof(buf));
        ssize_t recv_size = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if(recv_size < 0)
        {
            perror("recv");
            continue;
        }
        else if(recv_size == 0)
        {
            printf("peer close connect\n");
            close(sockfd);
            exit(1);
        }

        printf("%s\n", buf);

        sleep(1);
    }

    close(sockfd);

    return 0;
}
