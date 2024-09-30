#include <csignal>
#include <cstring>
#include <stdio.h>
#include <type_traits>
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
#include "logserver_manager.h"

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
    logclient->set_process_name("cassobee");

    auto* looper = reactor::get_instance();
    looper->init();
    looper->add_signal(SIGUSR1, sigusr1_handler);
    std::thread timer_thread = start_threadpool_and_timer();

    auto logservermgr = logserver_manager::get_instance();
    logservermgr->init();
    client(logservermgr);
    logclient->set_logserver(logservermgr);

    auto clientmgr = client_manager::get_instance();
    clientmgr->init();
    server(clientmgr);

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

    // local_log("type_name:%s", cassobee::type_name_string<int>().data());
    // local_log("type_name:%s", cassobee::type_name_string<cassobee::logger>().data());
    // local_log("type_name:%s", cassobee::short_type_name_string<cassobee::logger>().data());

    // local_log("run here:%s.", address_factory::get_instance()->infomation().data());

    // address_factory::create("ipv4_address", "0.0.0.0", 8888);
    // address_factory::create2<"ipv4_address">("0.0.0.0", 8888);

    looper->run();
    timer_thread.join();
    TRACELOG("process exit normally...\n");
    return 0;
}
