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

void start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    threadpool::get_instance()->start_timer_thread();
}

void sig_handler(int signum)
{
    threadpool::get_instance()->stop();
    printf("recv signal end process...\n");
}

int main()
{
    if(signal(SIGUSR1, sig_handler) == SIG_ERR)
    {
        printf("failed to register signal handler.\n");
        return -1;
    }

    start_threadpool_and_timer();
    //timewheel::get_instance()->init(10);
    timewheel::get_instance()->add_timer(false, 5, -1, [](void*){ printf("nowtime1: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5, 10, [](void*){ printf("nowtime2: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5, -1, [](void*){ printf("nowtime3: %ld.\n", systemtime::get_time()); return false; }, nullptr);
    //timewheel::get_instance()->run();
    sleep(30000);
    return 0;
}
