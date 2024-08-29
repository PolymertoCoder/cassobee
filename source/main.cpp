#include <csignal>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <thread>

#include "config.h"
#include "marshal.h"
#include "reactor.h"
#include "stringfy.h"
#include "address.h"
#include "threadpool.h"
#include "timewheel.h"
#include "systemtime.h"
#include "log.h"
#include "traits.h"
#include "common.h"
#include "factory.h"
#include "client_manager.h"

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
    add_timer(false, 5000, -1, [](void*){ local_log("timer1 nowtime1: %ld.", systemtime::get_time()); return true; }, nullptr);
    add_timer(false, 5000, 10, [](void*){ local_log("timer2 nowtime2: %ld.", systemtime::get_time()); return true; }, nullptr);
    add_timer(false, 5000, -1, [](void*){ local_log("timer3 nowtime3: %ld.", systemtime::get_time()); return false; }, nullptr);

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

    // std::vector<int> vec;
    // std::set<int> set;
    // std::map<int, int> map;
    // std::unordered_set<int> uset;
    // std::unordered_map<int, int> umap;
    // for(size_t i = 0; i < 10; ++i)
    // {
    //     vec.push_back(rand(70000, 4000000) + i * 7321);
    //     set.insert(rand(70000, 4000000) + i * 7321);
    //     map.emplace(i, rand(70000, 4000000) + i * 7321);
    //     uset.insert(rand(70000, 4000000) + i * 7321);
    //     umap.emplace(i, rand(70000, 4000000) + i * 7321);
    // }

    // std::vector<int> vec2;
    // std::set<int> set2;
    // std::map<int, int> map2;
    // std::unordered_set<int> uset2;
    // std::unordered_map<int, int> umap2;

    // {
    //     octetsstream os; os << vec; os >> vec2;
    //     printf("vec2 %s\n", cassobee::to_string(vec2).data());
    // }
    // {
    //     octetsstream os; os << set; os >> set2;
    //     printf("set2 %s\n", cassobee::to_string(set2).data());
    // }
    // {
    //     octetsstream os; os << map; os >> map2;
    //     printf("map2 %s\n", cassobee::to_string(map2).data());
    // }
    // {
    //     octetsstream os; os << uset; os >> uset2;
    //     printf("uset2 %s\n", cassobee::to_string(uset2).data());
    // }
    // {
    //     octetsstream os; os << umap; os >> umap2;
    //     printf("umap2 %s\n", cassobee::to_string(umap2).data());
    // }

    // local_log("type_name:%s", cassobee::type_name<int>());
    // local_log("type_name:%s", cassobee::type_name<cassobee::logger>());
    // local_log("type_name:%s", cassobee::short_type_name<cassobee::logger>());

    local_log("run here:%s.", address_factory::get_instance()->infomation().data());

    // const char* str{"ipv4_address"};
    // address_factory::get_instance()->create2<str>("0.0.0.0", 8888);

    auto clientmgr = new client_manager;
    clientmgr->init();
    server(clientmgr);

    reactor::get_instance()->run();
    timer_thread.join();
    printf("process exit normally...\n");
    return 0;
}
