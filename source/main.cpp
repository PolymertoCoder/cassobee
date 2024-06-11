#include "config.h"
#include "reactor.h"
#include "threadpool.h"
#include "delegate.h"
#include "timewheel.h"
#include "systemtime.h"
#include "log.h"
#include "marshal.h"
#include <climits>
#include <csignal>
#include <unordered_set>

std::thread start_threadpool_and_timer()
{
    threadpool::get_instance()->start();
    if(reactor::get_instance()->use_timer_thread())
    {
        return std::thread([]()
        {
            local_log("timer thread tid:%d.", gettid());
            timewheel::get_instance()->init();
            timewheel::get_instance()->run();
        });
    }
    return {};
}

bool sigusr1_handler(int signum)
{
    add_timer(3000, []()
    {
        timewheel::get_instance()->stop();
        threadpool::get_instance()->stop();
        reactor::get_instance()->stop();
        printf("recv signal end process...");
        return false;
    });
    return true;
}

int main()
{
    config::get_instance()->init("/home/qinchao/cassobee/config");
    cassobee::log_manager::get_instance()->init();
    cassobee::log_manager::set_process_name("cassobee");
    add_signal(SIGUSR1, sigusr1_handler);
    reactor::get_instance()->init();
    std::thread timer_thread = start_threadpool_and_timer();
    local_log("nowtime1=%ld", systemtime::get_microseconds());
    sleep(3);
    local_log("nowtime2=%ld", systemtime::get_microseconds());
    // add_timer(false, 5000, -1, [](void*){ local_log("timer1 nowtime1: %ld.", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, 10, [](void*){ local_log("timer2 nowtime2: %ld.", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, -1, [](void*){ local_log("timer3 nowtime3: %ld.", systemtime::get_time()); return false; }, nullptr);

    add_timer(1000, [](){ DEBUGLOG("DEBUG=%d", 10); return true; });
    add_timer(2000, [](){ INFOLOG("INFO=%d", 10);   return true; });
    add_timer(1500, [](){ WARNLOG("WARN=%d", 10);   return true; });
    add_timer(2000, [](){ ERRORLOG("ERROR=%d", 10); return true; });
    add_timer(2500, [](){ FATALLOG("FATAL=%d", 10); return true; });
    // while(true)
    // {
    //     threadpool::get_instance()->add_task(rand(0, 3), [](){ DEBUGLOG("DEBUG=%s", "多线程测试"); });
    //     threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
    //     threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
    //     threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
    //     usleep(1000);
    // }

    std::vector<int> vec;
    std::set<int> set;
    std::map<int, int> map;
    std::unordered_set<int> uset;
    std::unordered_map<int, int> umap;
    for(size_t i = 0; i < 10; ++i)
    {
        vec.push_back(rand(INT_MIN, INT_MAX));
        set.insert(rand(INT_MIN, INT_MAX));
        map.emplace(i, rand(INT_MIN, INT_MAX));
        uset.insert(rand(INT_MIN, INT_MAX));
        umap.emplace(i, rand(INT_MIN, INT_MAX));
    }
    octetsstream os; //os << vec;
    local_log("std::is_standard_layout_v<T> %d", std::is_standard_layout_v<std::vector<int>>);

    std::vector<int> vec2;
    std::set<int> set2;
    std::map<int, int> map2;
    std::unordered_set<int> uset2;
    std::unordered_map<int, int> umap2;
    for(size_t i = 0; i < 10; ++i)
    {

    }


    reactor::get_instance()->run();
    timer_thread.join();
    printf("process exit normally...\n");
    return 0;
}
