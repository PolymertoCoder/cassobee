#include "reactor.h"
#include "threadpool.h"
#include "delegate.h"
#include "timewheel.h"
#include "systemtime.h"

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

int main()
{
    //start_threadpool_and_timer();
    timewheel::get_instance()->init(10);
    timewheel::get_instance()->add_timer(false, 5, -1, [](void*){ printf("nowtime1: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5, 10, [](void*){ printf("nowtime2: %ld.\n", systemtime::get_time()); return true; }, nullptr);
    timewheel::get_instance()->add_timer(false, 5, -1, [](void*){ printf("nowtime3: %ld.\n", systemtime::get_time()); return false; }, nullptr);
    timewheel::get_instance()->run();
    //threadpool::get_instance()->stop();
    return 0;
}
