#include "reactor.h"
#include "threadpool.h"
#include "delegate.h"
#include "timewheel.h"
#include "systemtime.h"
#include <csignal>

bool timetask(void* param)
{
    printf("nowtime: %ld.\n", systemtime::get_time());
    return true;
}

std::thread start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    return std::thread([]()
    {
        timewheel::get_instance()->init();
        timewheel::get_instance()->run();
    });
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
    std::thread timer_thread = start_threadpool_and_timer();
    timewheel::get_instance()->add_timer(false, 5000, -1, [](void*){ printf("nowtime1: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5000, 10, [](void*){ printf("nowtime2: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5000, -1, [](void*){ printf("nowtime3: %ld.\n", systemtime::get_time()); return false; }, nullptr);
    reactor::get_instance()->run();
    timer_thread.join();
    return 0;
}
