#include "macros.h"
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
    add_signal(SIGUSR1, sigusr1_handler);
    reactor::get_instance()->init();
    std::thread timer_thread = start_threadpool_and_timer();
    cassobee::log_manager::get_instance()->init();
    cassobee::log_manager::set_process_name("cassobee");
    // printf("nowtime1=%ld\n", systemtime::get_microseconds());
    sleep(3);
    // printf("nowtime2=%ld\n", systemtime::get_microseconds());
    // add_timer(false, 5000, -1, [](void*){ printf("timer1 nowtime1: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, 10, [](void*){ printf("timer2 nowtime2: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, -1, [](void*){ printf("timer3 nowtime3: %ld.\n", systemtime::get_time()); return false; }, nullptr);

    DEBUGLOG("a=%d", 10);
    INFOLOG("a=%d", 10);
    WARNLOG("a=%d", 10);
    ERRORLOG("a=%d", 10);
    FATALLOG("a=%d", 10);
    // add_timer(1000, [](){ DEBUGLOG("DEBUG=%d", 10); return true; });
    // add_timer(2000, [](){ INFOLOG("INFO=%d", 10);   return true; });
    // add_timer(3000, [](){ WARNLOG("WARN=%d", 10);   return true; });
    // add_timer(4000, [](){ ERRORLOG("ERROR=%d", 10); return true; });
    // add_timer(5000, [](){ FATALLOG("FATAL=%d", 10); return true; });
    reactor::get_instance()->run();
    timer_thread.join();
    return 0;
}
