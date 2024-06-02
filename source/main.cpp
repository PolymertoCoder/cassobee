#include "config.h"
#include "reactor.h"
#include "threadpool.h"
#include "delegate.h"
#include "timewheel.h"
#include "systemtime.h"
#include "log.h"
#include <csignal>

bool timetask(void* param)
{
    printf("nowtime: %ld.\n", systemtime::get_time());
    return true;
}

std::thread start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    if(reactor::get_instance()->use_timer_thread())
    {
        return std::thread([]()
        {
            printf("timer thread tid:%d.", gettid());
            timewheel::get_instance()->init();
            timewheel::get_instance()->run();
        });
    }
    return {};
}

bool sigusr1_handler(int signum)
{
    timewheel::get_instance()->stop();
    threadpool::get_instance()->stop();
    reactor::get_instance()->stop();
    printf("recv signal end process...\n");
    return true;
}

int main()
{
    config::get_instance()->init("/home/qinchao/cassobee/config");
    add_signal(SIGUSR1, sigusr1_handler);
    reactor::get_instance()->init();
    std::thread timer_thread = start_threadpool_and_timer();
    cassobee::log_manager::get_instance()->init();
    cassobee::log_manager::set_process_name("cassobee");
    //printf("nowtime1=%ld\n", systemtime::get_microseconds());
    sleep(3);
    //printf("nowtime2=%ld\n", systemtime::get_microseconds());
    // add_timer(false, 5000, -1, [](void*){ printf("timer1 nowtime1: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, 10, [](void*){ printf("timer2 nowtime2: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, -1, [](void*){ printf("timer3 nowtime3: %ld.\n", systemtime::get_time()); return false; }, nullptr);

    //add_timer(1000,  [](){ DEBUGLOG("DEBUG=%d", 10); return true; });
    //add_timer(2000, [](){ INFOLOG("INFO=%d", 10);   return true; });
    //add_timer(1500, [](){ WARNLOG("WARN=%d", 10);   return true; });
    //add_timer(2000, [](){ ERRORLOG("ERROR=%d", 10); return true; });
    //add_timer(2500, [](){ FATALLOG("FATAL=%d", 10); return true; });
    while(true)
    {
        threadpool::get_instance()->add_task(rand(0, 3), [](){ DEBUGLOG("DEBUG=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
        usleep(1000);
    }
    reactor::get_instance()->run();
    timer_thread.join();
    return 0;
}
