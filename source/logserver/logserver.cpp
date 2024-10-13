#include <csignal>
#include "config.h"
#include "reactor.h"
#include "session_manager.h"
#include "timewheel.h"
#include "threadpool.h"
#include "log_manager.h"
#include "logclient_manager.h"

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

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        printf("Usage: %s --config <config filepath>...\n", argv[0]);
        return -1;
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

    auto logclient = cassobee::logclient::get_instance();
    logclient->init();
    logclient->set_process_name("logserver");

    auto* looper = reactor::get_instance();
    looper->init();
    looper->add_signal(SIGUSR1, sigusr1_handler);
    std::thread timer_thread = start_threadpool_and_timer();

    cassobee::log_manager::get_instance()->init();

    auto logclientmgr = logclient_manager::get_instance();
    logclientmgr->init();
    server(logclientmgr);

    while(true)
    {
        threadpool::get_instance()->add_task(rand(0, 3), [](){ DEBUGLOG("DEBUG=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        usleep(1000);
    }
    
    looper->run();
    timer_thread.join();
    TRACELOG("process exit normally...\n");
    return 0;
}