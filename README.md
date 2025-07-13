# cassobee 项目说明

## 项目简介
cassobee 是一个 Linux 高性能 C++ 服务器开发框架，支持事件驱动、日志、定时器、线程池、对象池、数据库、HTTP服务器/客户端、缓存、协议扩展、命令行工具、监控等功能，适合构建高并发网络服务。

## 目录结构
```
bin/           # 启动脚本
build/         # 编译中间产物
cmake/         # cmake 脚本
  fastbuild_gen.cmake # 根据CMakeLists.txt生成FastBuild编译过程文件
config/        # 配置文件
debug/         # 调试目录（可执行程序/库文件）
  logdir/      # 日志文件目录
  libs/        # 依赖库文件
  可执行程序...
doc/           # 文档
progen/        # 协议定义
source/        # 源代码目录
  bee/         # 主功能库
    cache/     # 缓存模块（TODO）
    cli/       # 命令行模块
    common/    # 公共模块
    database/  # 数据库模块
    format/    # 格式化、字符流模块
    http/      # HTTP模块
    io/        # 网络IO模块
    log/       # 日志模块
    lua/       # Lua脚本引擎模块（TODO）
    meta/      # 反射模块（TODO）
    monitor/   # 监控模块
    security/  # 安全模块
    thread/    # 多线程模块
    traits/    # 元编程、concept
    utility/   # 通用工具模块
  casso/       # cassobee主程序（主服务/演示）
  client/      # client测试程序
  logserver/   # 日志服务端
  protocol/    # 生成的协议文件
    include/   # 协议头文件
    source/    # 协议源文件
    state/     # 各服务间注册的协议状态文件
thirdparty/    # 第三方库目录
tools/         # 工具脚本
```

## 依赖与编译

### 1. 拉取代码
```bash
git clone https://gitee.com/qinchao11/cassobee.git
cd cassobee
```

### 2. 安装依赖
```bash
cd tools
./install
up # 更新
build_thirdparty # 编译第三方库
cd ..
```

### 3. 编译
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release # whatever Release or Debug
mk # 编译
```

## 脚本说明
- **install**：一键设置环境变量、依赖、常用目录别名、自动补全，初始化 telegraf 数据保留策略。
- **build_thirdparty**：自动编译所有第三方依赖库。
- **ca/ss/tt/uu/cfg/pp**：快速跳转到项目根目录/source/tools/bee/config/progen/目录下。
- **mk**：自动生成配置、进入 build 目录并编译主项目。
- **up**：更新项目代码、第三方库。
- **genconf**：根据 config/tpl 下模板生成所有配置文件，并自动生成 SSL 证书。
- **pg**：协议代码生成（将 progen/*.xml 自动生成到 source/protocol/）。
- **rs/ks/rsall/ksall**：服务管理脚本，支持单服务/全部服务的优雅重启与关闭。
- **psall**：彩色显示所有 cassobee 相关服务进程信息。
- **monitor_proc**：采集指定进程的 oncpu/offcpu 性能数据并生成火焰图。

---

## 各程序启动与用法

### cassobee 服务端测试程序

**主要流程（main.cpp 摘要）：**
```cpp
// 1. 加载配置
config::get_instance()->init("config/cassobee.conf");

// 2. 日志客户端初始化
auto logclient = bee::logclient::get_instance();
logclient->init();
logclient->set_process_name("cassobee");

// 3. 各核心服务初始化
reactor::get_instance()->init();
reactor::get_instance()->add_signal(SIGUSR1, sigusr1_handler);
std::thread timer_thread = start_threadpool_and_timer();

logserver_manager::get_instance()->init();
logserver_manager::get_instance()->connect();
logclient->set_logserver(logserver_manager::get_instance());

client_manager::get_instance()->init();
client_manager::get_instance()->listen();

auto http_server = std::make_unique<httpserver>();
http_server->init();
http_server->listen();

// 4. 各种定时器/日志演示
add_timer(1000, [](){ DEBUGLOG("DEBUG=%d", 10); return true; });
add_timer(2000, [](){ ERRORLOGF("ERROR={}", 20); return false; });
DEBUGLOG << "DEBUG=" << 10;
ERRORLOG << "ERROR=" << 20;

// 5. 启动主事件循环
reactor::get_instance()->run();
timer_thread.join();
```

**信号优雅退出：**
```cpp
reactor::get_instance()->add_signal(SIGUSR1, sigusr1_handler);
// kill -USR1 <pid> 可优雅关闭
```

---

### 2. client 测试程序

**启动命令：**
```bash
rs client
# 或带命令行交互
./client -with-cmd
```

**主要流程（client.cpp 摘要）：**
```cpp
// 1. 可选命令行交互线程
if(argc >= 2 && strcmp(argv[1], "-with-cmd") == 0) run_cli();

// 2. 加载配置
config::get_instance()->init("config/client.conf");

// 3. 日志客户端初始化
auto logclient = bee::logclient::get_instance();
logclient->init();
logclient->set_process_name("client");

// 4. 各核心服务初始化
reactor::get_instance()->init();
reactor::get_instance()->add_signal(SIGUSR1, sigusr1_handler);
std::thread timer_thread = start_threadpool_and_timer();

logserver_manager::get_instance()->init();
logserver_manager::get_instance()->connect();
logclient->set_logserver(logserver_manager::get_instance());

server_manager::get_instance()->init();
server_manager::get_instance()->connect();

auto* http_client = new httpclient();
http_client->init();

// 5. 定时发起 HTTP 请求
add_timer(1000, [http_client](){
    http_client->get("/test/hello", [](int status, httprequest* req, httpresponse* rsp) {
        local_log("receive httpresponse status:%d, callback run.", status);
    });
    return true;
});

// 6. 定时发起 RPC 请求
add_timer(1000, [servermgr](){
    auto rpc = rpc_callback<ExampleRPC>::call({1, 2},
        // 回调函数
        [](rpcdata* argument, rpcdata* result){
            auto arg = (ExampleRPCArg*)argument;
            auto res = (EmptyRpcRes*)result;
            local_log_f("rpc callback received: param1:{} param2:{}, result:{}", arg->param1, arg->param2, res->sum);
        },
        // 超时回调
        [](rpcdata* argument){
            auto arg = (ExampleRPCArg*)argument;
            local_log_f("rpc timeout for: param1:{} param2:{}", arg->param1, arg->param2);
        }
    );
    servermgr->send(*rpc);
    return true;
});

// 7. 启动主事件循环
reactor::get_instance()->run();
timer_thread.join();
```

---

### 3. logserver 日志服务

**启动命令：**
```bash
rs logserver
```

**主要流程（logserver.cpp 摘要）：**
```cpp
// 1. 加载配置
config::get_instance()->init("config/logserver.conf");

// 2. 日志客户端初始化
auto logclient = bee::logclient::get_instance();
logclient->init();
logclient->set_process_name("logserver");

// 3. 各核心服务初始化
reactor::get_instance()->init();
reactor::get_instance()->add_signal(SIGUSR1, sigusr1_handler);
std::thread timer_thread = start_threadpool_and_timer();

bee::log_manager::get_instance()->init();
logclient_manager::get_instance()->init();
logclient_manager::get_instance()->listen();

// 4. 启动主事件循环
reactor::get_instance()->run();
```

---

## bee 库常用 API 及用法

### 1. 配置
```cpp
#include "config.h"
auto cfg = bee::config::get_instance();
cfg->init("config/cassobee.conf");
std::string host = cfg->get<std::string>("server", "host");
int port = cfg->get<int>("server", "port");
...
```

### 2. 日志
```cpp
DEBUGLOG("Application started, now:%lld.", systemtime::get_time());
ERRORLOGF("Error occurred: {}.", error_msg);
```

### 3. 定时器
```cpp
add_timer(1000, [](){ DEBUGLOG("定时器触发"); return true; });
```

### 4. 线程池
```cpp
threadpool::get_instance()->add_task(0/*thread group idx*/, [](int i){
    local_log_f("threadpool task run: {}", i);
});
```

### 5. 对象池
```cpp
bee::lockfree_objectpool<MyObj> pool(1000);
auto [idx, obj] = pool.alloc();
if(obj) { /*使用obj*/ pool.free(obj); }
```

### 6. Reactor 事件循环
```cpp
auto* looper = bee::reactor::get_instance();
looper->init();
looper->run();
```

### 7. 事件派发与观察者
```cpp
bee::event_dispatcher<int> dispatcher;
dispatcher.subscribe([](int v){ printf("observer1: %d\n", v); });
dispatcher.subscribe([](int v){ printf("observer2: %d\n", v); });
dispatcher.emit(100);
```

### 8. HTTP 客户端/服务端
```cpp
// 服务端
auto http_server = std::make_unique<httpserver>();
http_server->init();
http_server->listen();

// 客户端
auto* http_client = new httpclient();
http_client->init();
http_client->get("/test/hello", [](int status, httprequest* req, httpresponse* rsp) {
    local_log("receive httpresponse status:%d, callback run.", status);
});
```

### 9. 协议与序列化、codefield机制
- 协议定义在 `progen/*.xml`，通过 `tools/pg` 生成 C++ 协议类。
- 发送：通过 `session->send(protocol_obj)` 或 `session_manager->send(protocol_obj)`。
- 处理：在解包后将协议任务放入线程池等待处理，处理时会调用协议对应的run()方法
- 支持 codefield 位图机制：字段未设置时用默认值，序列化时自动忽略；小整数使用varint/zigzag编码，节省空间。
- 协议类和 rpcdata 支持自动序列化/反序列化，底层用 octetsstream（字节流）。
```cpp
// 发送
ExampleProtocol proto;
proto.set_field1(123); // field2未设置，序列化时不输出
session->send(proto);

// 协议处理
void ExampleProtocol::run(){
    printf("field1:%d field2:%f\n", field1, field2);
}

// 序列化/反序列化
bee::octetsstream os;
os << proto;
ExampleProtocol proto2;
os >> proto2;
```

### 10. RPC 调用
```cpp
auto rpc = rpc_callback<ExampleRPC>::call({/*arg...*/},
    [](rpcdata* argument, rpcdata* result){
        // 处理回调
    },
    [](rpcdata* argument){
        // 超时处理
    }
);
servermgr->send(*rpc);
```

### 11. 委托（Delegate）
```cpp
MULTICAST_DELEGATE_DECLEAR(on_event, int);
on_event.bind([](int v){ printf("event: %d\n", v); });
on_event += [](int v){ printf("event: %d\n", v + 1); };
on_event.broadcast(42);
```

### 12. CLI 命令扩展
```cpp
class mycmd : public bee::command {
public:
    mycmd(const std::string& name, const std::string& desc) : command(name, desc) {}
    void execute(const std::vector<std::string>& args) override {
        // 命令逻辑
    }
};
auto cli = bee::command_line::get_instance();
cli->add_command("mycmd", new mycmd("mycmd", "自定义命令"));
```

### 13. random 库
```cpp
int r = bee::random::range(0, 100);
double d = bee::random::generate<double>();
std::vector<int> candidates{0, 1, 2, 3, 4, 5};
int candidate = bee::random::choice(candidates);
std::vector<int> final_candidate = bee::random::select(candidates, 2);
bee::random::shuffle(candidates);
```

### 14. bee::ostringstream 字符串拼接
```cpp
bee::ostringstream oss;
oss << "value=" << 123 << ", str=" << "abc";
std::string s = oss.str();
```

### 15. 监控
```cpp
auto monitor = bee::monitor::get_instance();
monitor->init();
monitor->start(); // 由定时器定期搜集数据并输出
```

---

## 配置文件说明
- `config/cassobee.conf`：主服务配置（端口、线程数、数据库等）
- `config/logserver.conf`：日志服务配置
- `config/client.conf`：测试程序配置
- `config/tabledef.xml`：数据库表结构定义
- `config/tpl/*.conf.m4`：数据模板配置，通过genconf在config/下生成对应的.conf配置文件
- `config/grafana/system_dashboard.json`：Grafana系统监控面板配置
- `config/ssl/*.pem`：SSL证书文件
- `config/telegraf/telegraf.conf`：telegraf配置

---


## FAQ/常见问题

### 编译与依赖
- **Q: 编译报找不到依赖？**  
  A: 请先运行 `tools/install` 和 `tools/build_thirdparty`，确保所有依赖已安装。

### 协议与序列化
- **Q: 如何自定义协议？**  
  A: 在 `progen/` 下新增 xml，在xml中定义协议名、协议号、字段类型、默认值等，运行 `pg` 自动生成协议代码。
- **Q: 协议的 codefield 机制如何节省空间？**  
  A: 未设置的字段不会被序列化，反序列化时自动赋默认值，适合大协议和稀疏字段。另外，对小整数，会进行varint/zigzag编码节省空间。

### 日志
- **Q: 日志无法输出？**  
  A: 检查 `config/logserver.conf` 配置和日志服务是否启动。

### 监控
- **Q: 如何扩展监控指标？**  
  A: 通过 `monitor::add_metric`等接口自定义和上报监控数据。

### HTTP
- **Q: 如何扩展 HTTP 服务？**  
  A: 继承 `bee::servlet`，实现 `handle` 方法，并注册到对应 `httpserver::servlet_dispatcher`。

### 事件
- **Q: 如何实现事件派发和观察者模式？**  
  A: 使用 `event_dispatcher`，支持多观察者订阅和事件分发。

### CLI
- **Q: 如何添加自定义 CLI 命令？**  
  A: 继承 `bee::command`，实现 execute 方法，并通过 `command_line::add_command` 注册。

### 委托
- **Q: 委托(delegate)和事件派发有何区别？**  
  A: delegate 适合单一事件多回调，event_dispatcher 适合多类型事件多观察者。

---

如需更详细的代码示例、模块接口文档或有其他定制需求，请查阅源码或联系维护者。
