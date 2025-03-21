#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <arpa/inet.h>
#include "app_commands.h"
#include "app_command.h"
#include "command_line.h"
#include "session_manager.h"
#include "log.h"
#include "glog.h"
#include "reactor.h"
#include "threadpool.h"
#include "timewheel.h"

using namespace bee;

bool sigusr1_handler(int signum)
{
    add_timer(3000, []()
    {
        timewheel::get_instance()->stop();
        threadpool::get_instance()->stop();
        reactor::get_instance()->stop();
        printf("receive shutdown signal, end process...\n");
        return false;
    });
    return true;
}

int main()
{
#if 0
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

    while(true)
    {
        char buf[1024] = "i am client abcdef";
        send(sockfd, buf, strlen(buf), 0);
        printf("send...\n");

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
#elif 1
    auto logclient = bee::logclient::get_instance();
    logclient->init();
    logclient->set_process_name("client");

    auto cli = bee::command_line::get_instance();
    cli->add_command("config", new bee::config_command("config", "Load config"));
    cli->add_command("connect", new bee::connect_command("connect", "Connect server"));
    auto root_path   = fs::current_path().parent_path();
    auto config_file = root_path/"source/client/.cliinit";
    cli->execute_file(config_file);
    auto cli_thread = std::thread([cli](){ cli->run(); });

    auto* looper = reactor::get_instance();
    looper->init();
    looper->add_signal(SIGUSR1, sigusr1_handler);
    std::thread timer_thread = start_threadpool_and_timer();

    auto logservermgr = logserver_manager::get_instance();
    logservermgr->init();
    client(logservermgr);
    logclient->set_logserver(logservermgr);
    looper->run();

    timer_thread.join();
    cli_thread.join();

#endif

    return 0;
}
