# CassoBee 项目说明

## 项目简介
CassoBee 是一个 Linux 高性能 C++ 服务器开发框架，支持事件驱动、HTTP服务器/客户端、日志、数据库、协议扩展、命令行工具等功能，适合构建高并发网络服务和 Web 应用。

## 目录结构
```
source/
  bee/         # 主功能库
  casso/       # cassobee主程序（主服务/演示）
  client/      # client测试程序
  logserver/   # 日志服务端
  ...
config/        # 配置文件
progen/        # 协议定义
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
./build_thirdparty
cd ..
```

### 3. 编译
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
../tools/mk
```

---

## 各程序启动与用法

### 1. cassobee 主程序

**启动命令：**
```bash
cd source/casso
./cassobee --config ../../config/cassobee.conf
```

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
add_timer(2000, [](){ INFOLOG("INFO=%d", 10);   return true; });
add_timer(1500, [](){ WARNLOG("WARN=%d", 10);   return true; });
add_timer(2000, [](){ ERRORLOG("ERROR=%d", 10); return true; });
add_timer(2500, [](){ FATALLOG("FATAL=%d", 10); return true; });

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
cd source/client
./client --config ../../config/client.conf
# 或带命令行交互
./client -with-cmd --config ../../config/client.conf
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
        [](rpcdata* argument, rpcdata* result){
            auto arg = (ExampleRPCArg*)argument;
            auto res = (EmptyRpcRes*)result;
            local_log_f("rpc callback received: param1:{} param2:{}, result:{}", arg->param1, arg->param2, res->sum);
        },
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
cd source/logserver
./logserver --config ../../config/logserver.conf
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
timer_thread.join();
```

---

## bee 库常用 API 及用法

### 1. 配置与监控
```cpp
auto cfg = bee::config::get_instance();
cfg->init("config/cassobee.conf");

// 监控数据拓展
bee::monitor::get_instance()->add_metric("custom_metric", 0);
bee::monitor::get_instance()->inc("custom_metric");
bee::monitor::get_instance()->report();
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

### 4. 对象池
```cpp
bee::lockfree_objectpool<MyObj> pool(1000);
auto [idx, obj] = pool.alloc();
if(obj) { /* 使用obj */ pool.free(obj); }
```

### 5. Reactor 事件循环
```cpp
auto* looper = bee::reactor::get_instance();
looper->init();
looper->run();
```

### 6. 事件派发与观察者
```cpp
bee::event_dispatcher<int> dispatcher;
dispatcher.subscribe([](int v){ printf("observer1: %d\n", v); });
dispatcher.subscribe([](int v){ printf("observer2: %d\n", v); });
dispatcher.emit(100);
```

### 7. HTTP 客户端/服务端
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

### 8. 协议与序列化、codefield机制
- 协议定义在 `progen/*.xml`，通过 `tools/pg` 生成 C++ 协议类。
- 发送：通过 `session->send(protocol_obj)` 或 `servermgr->send(protocol_obj)`。
- 处理：在 manager/handler 注册回调，收到协议后自动分发。
- 支持 codefield 位图机制：字段未设置时用默认值，序列化时自动忽略，节省空间。
- 协议类和 rpcdata 支持自动序列化/反序列化，底层用 octetsstream。
```cpp
// 发送
ExampleProtocol proto;
proto.set_field1(123); // field2未设置，序列化时不输出
session->send(proto);

// 处理注册
server_manager::get_instance()->register_handler<ExampleProtocol>([](const ExampleProtocol& proto, session* sess){
    // 处理逻辑
});

// 序列化/反序列化
bee::octetsstream os;
os << proto;
ExampleProtocol proto2;
os >> proto2;
```

### 9. RPC 调用
```cpp
add_timer(1000, [servermgr](){
    auto rpc = rpc_callback<ExampleRPC>::call({1, 2},
        [](rpcdata* argument, rpcdata* result){
            // 处理回调
        },
        [](rpcdata* argument){
            // 超时处理
        }
    );
    servermgr->send(*rpc);
    return true;
});
```

### 10. 委托（Delegate）
```cpp
bee::delegate<void(int)> on_event;
on_event += [](int v){ printf("event: %d\n", v); };
on_event(42);
```

### 11. CLI 命令扩展
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

### 12. random 库
```cpp
int r = bee::random::rand(0, 100);
double d = bee::random::rand_double();
```

### 13. bee::ostringstream 字符串拼接
```cpp
bee::ostringstream oss;
oss << "value=" << 123 << ", str=" << "abc";
std::string s = oss.str();
```

---

## 配置文件说明
- `config/cassobee.conf`：主服务配置（端口、线程数、数据库等）
- `config/logserver.conf`：日志服务配置
- `config/client.conf`：测试程序配置
- `config/tabledef.xml`：数据库表结构定义

---

## tools 脚本工具说明
- `install`：自动安装依赖
- `build_thirdparty`：编译第三方库
- `mk`：快速编译主项目
- `pg`：协议代码生成（根据 progen/*.xml 自动生成协议头/源文件）
- 其余脚本详见 tools 目录注释或直接运行 `./脚本名 --help`

---

## FAQ/常见问题

### 编译与依赖
- **Q: 编译报找不到依赖？**  
  A: 请先运行 `tools/install` 和 `tools/build_thirdparty`，确保所有依赖已安装。

### 协议与序列化
- **Q: 如何自定义协议？**  
  A: 在 `progen/` 下新增 xml，运行 `tools/pg` 自动生成协议代码。
- **Q: 协议的 codefield 机制如何节省空间？**  
  A: 未设置的字段不会被序列化，反序列化时自动赋默认值，适合大协议和稀疏字段。

### 日志与监控
- **Q: 日志无法输出？**  
  A: 检查 `config/logserver.conf` 配置和日志服务是否启动。
- **Q: 如何扩展监控指标？**  
  A: 通过 `monitor::add_metric`、`inc`、`report` 等接口自定义和上报监控数据。

### HTTP/事件/对象池
- **Q: 如何扩展 HTTP 服务？**  
  A: 继承 `bee::servlet`，实现 `handle` 方法，并注册到对应 `httpserver::servlet_dispatcher`。
- **Q: 如何实现事件派发和观察者模式？**  
  A: 使用 `event_dispatcher`，支持多观察者订阅和事件分发。
- **Q: bee::lockfree_objectpool 有什么优势？**  
  A: 支持高并发下对象复用，减少内存分配和碎片化。

### CLI/委托/random
- **Q: 如何添加自定义 CLI 命令？**  
  A: 继承 `bee::command`，实现 execute 方法，并通过 `command_line::add_command` 注册。
- **Q: 委托(delegate)和事件派发有何区别？**  
  A: delegate 适合单一事件多回调，event_dispatcher 适合多类型事件多观察者。
- **Q: 如何生成高性能随机数？**  
  A: 使用 `bee::random::rand`、`rand_double` 等接口。

---

如需更详细的代码示例、模块接口文档或有其他定制需求，请查阅源码或联系维护者。
