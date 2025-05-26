#include <csignal>
#include <cstring>
#include <stdio.h>
#include <type_traits>
#include <unistd.h>
#include <functional>
#include <thread>
#include <ranges>

#include "config.h"
#include "marshal.h"
#include "reactor.h"
#include "remotelog.h"
#include "rpc.h"
#include "stringfy.h"
#include "address.h"
#include "threadpool.h"
#include "timewheel.h"
#include "systemtime.h"
#include "glog.h"
#include "glog.h"
#include "traits.h"
#include "common.h"
#include "factory.h"
#include "client_manager.h"
#include "logserver_manager.h"
#include "lr_map.h"
#include "format.h"
#include "util.h"
#include "ExampleRPC.h"

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

struct TestObject
{
    int value;
    double data[100];
    TestObject() : value(0) {}
};

void stress_test()
{
    lockfree_objectpool<TestObject> pool(1000);
    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 100000;

    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                auto [idx, obj] = pool.alloc();
                if (obj) {
                    obj->value = j;
                    pool.free(obj);
                }
            }
        });
    }

    for (auto& t : threads) t.join();
    local_log("Stress test completed");
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

    auto logclient = bee::logclient::get_instance();
    logclient->init();
    logclient->set_process_name("cassobee");

    auto* looper = reactor::get_instance();
    looper->init();
    looper->add_signal(SIGUSR1, sigusr1_handler);
    std::thread timer_thread = start_threadpool_and_timer();

    auto logservermgr = logserver_manager::get_instance();
    logservermgr->init();
    logservermgr->connect();
    logclient->set_logserver(logservermgr);

    auto clientmgr = client_manager::get_instance();
    clientmgr->init();
    clientmgr->listen();

    local_log("nowtime1=%ld", systemtime::get_microseconds());
    sleep(3);
    local_log("nowtime2=%ld", systemtime::get_microseconds());

    // int a = 1;
    // float b = 2.f;
    // std::string c = "hello world";
    // auto dview = std::views::iota(0, 11);
    // std::vector<int> d{dview.begin(), dview.end()};
    // DEBUGLOG << "logstream test, a:" << a << ", b:"<< b << ", c:" << c << ", d:" << d << "." << bee::endl;
    // DEBUGLOGF("logformat test, a:{}, b:{}, c:{} d:{}.", a, b, c, d);
    // DEBUGLOG("logformat test, a:%d, b:%f, c:%s, d:%s.", a, b, c.data(), bee::to_string(d).data());

    // add_timer(false, 5000, -1, [](void*){ local_log("timer1 nowtime1: %ld.", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, 10, [](void*){ local_log("timer2 nowtime2: %ld.", systemtime::get_time()); return true; }, nullptr);
    // add_timer(false, 5000, -1, [](void*){ local_log("timer3 nowtime3: %ld.", systemtime::get_time()); return false; }, nullptr);

    add_timer(1000, [](){ DEBUGLOG("DEBUG=%d", 10); return true; });
    add_timer(2000, [](){ INFOLOG("INFO=%d", 10);   return true; });
    add_timer(1500, [](){ WARNLOG("WARN=%d", 10);   return true; });
    add_timer(2000, [](){ ERRORLOG("ERROR=%d", 10); return true; });
    add_timer(2500, [](){ FATALLOG("FATAL=%d", 10); return true; });

    // std::thread testlog_thread([]()
    // {
    //     while(true)
    //     {
    //         threadpool::get_instance()->add_task(rand(0, 3), [](){ DEBUGLOG("DEBUG=%s", "多线程测试"); });
    //         threadpool::get_instance()->add_task(rand(0, 3), [](){ INFOLOG("INFO=%s", "多线程测试"); });
    //         threadpool::get_instance()->add_task(rand(0, 3), [](){ WARNLOG("WARN=%s", "多线程测试"); });
    //         threadpool::get_instance()->add_task(rand(0, 3), [](){ ERRORLOG("ERROR=%s", "多线程测试"); });
    //         // local_log("testlog_thread");
    //         std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //     }
    // });
    // testlog_thread.detach();

    // stress_test();

    // add_timer(1000, [](){ static int i = 0; DEBUGLOG("DEBUG=%d", i++); return true; });

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
    //     printf("vec2 %s\n", bee::to_string(vec2).data());
    // }
    // {
    //     octetsstream os; os << set; os >> set2;
    //     printf("set2 %s\n", bee::to_string(set2).data());
    // }
    // {
    //     octetsstream os; os << map; os >> map2;
    //     printf("map2 %s\n", bee::to_string(map2).data());
    // }
    // {
    //     octetsstream os; os << uset; os >> uset2;
    //     printf("uset2 %s\n", bee::to_string(uset2).data());
    // }
    // {
    //     octetsstream os; os << umap; os >> umap2;
    //     printf("umap2 %s\n", bee::to_string(umap2).data());
    // }

    // local_log("type_name:%s", bee::type_name_string<int>().data());
    // local_log("type_name:%s", bee::type_name_string<bee::logger>().data());
    // local_log("type_name:%s", bee::short_type_name_string<bee::logger>().data());

    // local_log("run here:%s.", address_factory::get_instance()->infomation().data());

    // address_factory::create("ipv4_address", "0.0.0.0", 8888);
    // address_factory::create2<"ipv4_address">("0.0.0.0", 8888);

    // lr_map<int, int> map;
    // map.emplace(0, 1);
    // map.emplace(1, 2);
    // map.emplace(3, 4);
    // map.emplace(5, 6);
    // printf("empty:%d 1:%d, 5:%d\n", map.empty(), map[1], map[5]);
    // map.clear();
    // printf("empty:%d\n", map.empty());

    // octetsstream os;
    // bee::remotelog prot(123, {});
    // prot.logevent.set_all_fields();
    // prot.encode(os);
    // auto temp = protocol::decode(os, nullptr);
    // auto prot2 = dynamic_cast<bee::remotelog*>(temp);
    // printf("prot2 loglevel:%d\n", prot2->loglevel),

    // auto test_http_encode_decode = [](const std::string& str)
    // {
    //     std::string encoded = bee::util::url_encode(str);
    //     std::string decoded = bee::util::url_decode(encoded);
    //     DEBUGLOGF("encoded: {}, decoded: {}, decode_result:{}", encoded, decoded, decoded==str);
    // };
    // test_http_encode_decode(""); // 空字符串
    // test_http_encode_decode(""); // 纯ASCII字符（无需编码）
    // test_http_encode_decode("it is encodeurl decodeurl test="); // 空格
    // test_http_encode_decode(":/?#[]@!$&'()*+,;="); // RFC 3986保留字符
    // test_http_encode_decode("^\\`{|}"); // 需要编码的特殊符号
    // test_http_encode_decode("中文"); // 中文
    // test_http_encode_decode("%41%42%43"); // 十六进制
    // test_http_encode_decode("%zz"); // 无效十六进制字符
    // test_http_encode_decode("%1"); // 不完整的百分比编码
    // test_http_encode_decode("%%20"); // 双重%开头

    ExampleRPC rpc;
    auto rpc2 = rpc;

    rpc_callback<ExampleRPC>::call(ExampleRpcData{0.5, {}},
        [](rpcdata* argument, rpcdata* result)
        {
            auto arg = (ExampleRpcData*)argument;
            auto res = (EmptyRpcData2*)result;

            local_log("rpc callback received: value=%f, text=%d", arg->fieldA, res->code);
        },
        [](rpcdata* argument)
        {
            auto arg = (ExampleRpcData*)argument;
            local_log("rpc timeout for value=%f", arg->fieldA);
        }
    );

    looper->run();
    timer_thread.join();
    local_log("process exit normally...\n");
    return 0;
}
