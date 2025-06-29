#include <memory>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <arpa/inet.h>
#include <format.h>

#include "app_commands.h"
#include "app_command.h"
#include "command_line.h"
#include "httpprotocol.h"
#include "log.h"
#include "session_manager.h"
#include "logserver_manager.h"
#include "glog.h"
#include "glog.h"
#include "reactor.h"
#include "threadpool.h"
#include "timewheel.h"
#include "ExampleRPC.h"
#include "httpclient.h" 

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

std::thread run_cli()
{
    auto cli = bee::command_line::get_instance();
    cli->add_command("config", new bee::config_command("config", "Load config"));
    cli->add_command("connect", new bee::connect_command("connect", "Connect server"));
    auto root_path   = fs::current_path().parent_path();
    auto config_file = root_path/"source/client/.cliinit";
    cli->execute_file(config_file.c_str());
    auto cli_thread = std::thread([cli](){ cli->run(); });
    return cli_thread;
}

int main(int argc, char* argv[])
{
    std::thread cli_thread;
    if(argc >= 2 && strcmp(argv[1], "-with-cmd") == 0)
    {
        cli_thread = run_cli();
    }
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "--config") == 0)
        {
            printf("config filepath:%s\n", argv[i+1]);
            config::get_instance()->init(argv[++i]);
        }
        else
        {
            printf("unprocessed arg %s...\n", argv[i]);
        }
    }

    auto logclient = bee::logclient::get_instance();
    logclient->init();
    logclient->set_process_name("client");

    auto* looper = reactor::get_instance();
    looper->init();
    looper->add_signal(SIGUSR1, sigusr1_handler);
    std::thread timer_thread = start_threadpool_and_timer();

    auto logservermgr = logserver_manager::get_instance();
    logservermgr->init();
    logservermgr->connect();
    logclient->set_logserver(logservermgr);

    auto servermgr = server_manager::get_instance();
    servermgr->init();
    servermgr->connect();

    auto* http_client = new httpclient();
    http_client->init();

    // 发送并发请求
    add_timer(1000, [http_client]()
    {
        http_client->get("/test/hello", [](int status, httprequest* req, httpresponse* rsp) {
            local_log("receive httpresponse status:%d, callback run.", status);
        });
        return true;
    });
    

    // TIMERID timerid = add_timer(1000, [servermgr]()
    // {
    //     auto rpc = rpc_callback<ExampleRPC>::call({1, 2},
    //         [](rpcdata* argument, rpcdata* result)
    //         {
    //             auto arg = (ExampleRPCArg*)argument;
    //             auto res = (EmptyRpcRes*)result;

    //             local_log_f("rpc callback received: param1:{} param2:{}, result:{}", arg->param1, arg->param2, res->sum);
    //         },
    //         [](rpcdata* argument)
    //         {
    //             auto arg = (ExampleRPCArg*)argument;
    //             local_log_f("rpc timeout for: param1:{} param2:{}", arg->param1, arg->param2);
    //         }
    //     );
    //     servermgr->send(*rpc);
    //     return true;
    // });
    // local_log("timerid: %d", timerid);

    looper->run();
    timer_thread.join();
    if(cli_thread.joinable()) cli_thread.join();
    local_log("process client exit normally...");
    return 0;
}
