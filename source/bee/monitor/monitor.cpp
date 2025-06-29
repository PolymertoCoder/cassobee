#include "monitor.h"
#include "metric.h"
#include "metric_collector.h"
#include "metric_exporter.h"
#include "cpu_collector.h"
#include "disk_collector.h"
#include "mem_collector.h"
#include "proc_collector.h"
#include "net_collector.h"
#include "lock.h"
#include "glog.h"
#include "config.h"
#include "common.h"
#include "reactor.h"
#ifdef _REENTRANT
#include "threadpool.h"
#endif

namespace bee
{

void monitor_engine::start()
{
    auto* cfg = bee::config::get_instance();

    std::string exporter_names = cfg->get("monitor", "exporters");
    assert(exporter_names.size());
    for(const auto& exporter_name : split(exporter_names, ", "))
    {
        if(exporter_name == "influx")
        {
            register_exporter(new influx_exporter);
        }
        else if(exporter_name == "prometheus")
        {
            // TODO: prometheus exporter
            // register_exporter(new prometheus_exporter);
        }
    }
    assert(_exporters.size());

    std::string collector_names = cfg->get("monitor", "collectors");
    assert(collector_names.size());
    for(const auto& collector_name : split(collector_names, ", "))
    {
        if(collector_name == "cpu")
        {
            register_collector(new cpu_collector);
        }
        else if(collector_name == "memory")
        {
            register_collector(new mem_collector);
        }
        else if(collector_name == "disk")
        {
            register_collector(new disk_collector);
        }
        else if(collector_name == "process")
        {
            register_collector(new proc_collector);
        }
        else if(collector_name == "network")
        {
            register_collector(new net_collector);
        }
    }
    assert(_exporters.size());

    TIMETYPE interval = cfg->get<TIMETYPE>("monitor", "interval", 1000); // ms
    _timerid = add_timer(interval, [this]()
    {
    #ifdef _REENTRANT
        threadpool::get_instance()->add_task(thread_group_idx(), this);
    #else
        run();
    #endif
        return true;
    });

    local_log("monitor engine started.");
}

void monitor_engine::stop()
{
    del_timer(_timerid);
}

void monitor_engine::run()
{
    collect_all();
    export_all();
}

void monitor_engine::register_collector(metric_collector* collector)
{
    ASSERT(collector);
    bee::mutex::scoped l(_locker);

    if(auto iter = _collectors.find(collector->name()); iter == _collectors.end())
    {
        _collectors.insert(std::make_pair(collector->name(), collector));
    }
    else
    {
        local_log_f("collector already registered: {}", collector->name());
    }
}

void monitor_engine::unregister_collector(const std::string& name)
{
    bee::mutex::scoped l(_locker);
    _collectors.erase(name);
}

void monitor_engine::unregister_collector(metric_collector* collector)
{
    ASSERT(collector);
    unregister_collector(collector->name());
}

metric_collector* monitor_engine::find_collector(const std::string& name) const
{
    bee::mutex::scoped l(_locker);
    auto iter = _collectors.find(name);
    return iter != _collectors.end() ? iter->second : nullptr;
}

// 注册指标
void monitor_engine::register_metric(metric* metric)
{
    bee::mutex::scoped l(_locker);
    _metrics.push_back(metric);
}

// 注册导出器
void monitor_engine::register_exporter(metric_exporter* exporter)
{
    bee::mutex::scoped l(_locker);
    _exporters.push_back(exporter);
}

// 清空注册中心
void monitor_engine::clear()
{
    bee::mutex::scoped l(_locker);
    _collectors.clear();
    _metrics.clear();
    _exporters.clear();
}

// 导出所有指标
void monitor_engine::export_to(metric_exporter* exporter)
{
    bee::mutex::scoped l(_locker);
    for(auto metric : _metrics)
    {
        metric->export_to(*exporter);
    }
    _metrics.clear();
}

// 导出所有指标到所有导出器
void monitor_engine::export_all()
{
    bee::mutex::scoped l(_locker);
    for(auto exporter : _exporters)
    {
        for(auto metric : _metrics)
        {
            metric->export_to(*exporter);
        }
    }
    _metrics.clear();
}

// 收集所有指标
void monitor_engine::collect_all()
{
    bee::mutex::scoped l(_locker);
    for(auto& [_, collector] : _collectors)
    {
        
    }
}

// 监控核心引擎
// class MonitorCore {
// public:
//     MonitorCore(size_t queue_capacity = 10000) 
//         : data_queue_(queue_capacity), running_(false) {}

//     ~MonitorCore() {
//         stop();
//     }

//     void add_item(std::shared_ptr<MonitorItem> item) {
//         std::lock_guard<std::mutex> lock(items_mutex_);
//         items_.push_back(item);
//     }

//     void add_plugin(std::shared_ptr<OutputPlugin> plugin) {
//         std::lock_guard<std::mutex> lock(plugins_mutex_);
//         plugins_.push_back(plugin);
//     }

//     void start() {
//         if (running_) return;
        
//         running_ = true;
        
//         // 启动收集线程
//         collector_thread_ = std::thread(&MonitorCore::collector_loop, this);
        
//         // 启动处理线程
//         processor_thread_ = std::thread(&MonitorCore::processor_loop, this);
//     }

//     void stop() {
//         if (!running_) return;
        
//         running_ = false;
//         queue_cv_.notify_all();
        
//         if (collector_thread_.joinable()) {
//             collector_thread_.join();
//         }
        
//         if (processor_thread_.joinable()) {
//             processor_thread_.join();
//         }
        
//         // 刷新所有插件
//         std::lock_guard<std::mutex> lock(plugins_mutex_);
//         for (auto& plugin : plugins_) {
//             plugin->flush();
//         }
//     }

// private:
//     void collector_loop() {
//         while (running_) {
//             auto start_time = std::chrono::steady_clock::now();
            
//             // 收集所有监控项的数据
//             std::vector<DataBatch> all_batches;
            
//             {
//                 std::lock_guard<std::mutex> lock(items_mutex_);
//                 for (auto& item : items_) {
//                     try {
//                         DataBatch batch = item->collect();
//                         if (!batch.empty()) {
//                             all_batches.push_back(std::move(batch));
//                         }
//                     } catch (const std::exception& e) {
//                         std::cerr << "Error collecting from " << item->name() 
//                                   << ": " << e.what() << std::endl;
//                     }
//                 }
//             }
            
//             // 合并批次并放入队列
//             if (!all_batches.empty()) {
//                 DataBatch combined_batch;
//                 for (auto& batch : all_batches) {
//                     combined_batch.insert(combined_batch.end(), 
//                                          std::make_move_iterator(batch.begin()),
//                                          std::make_move_iterator(batch.end()));
//                 }
                
//                 while (running_ && !data_queue_.push(std::move(combined_batch))) {
//                     // 队列已满，等待并重试
//                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                 }
                
//                 queue_cv_.notify_one();
//             }
            
//             // 计算下一次收集时间
//             auto end_time = std::chrono::steady_clock::now();
//             auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                 end_time - start_time);
            
//             // 找出最短的间隔时间
//             int min_interval = 1000; // 默认1秒
//             {
//                 std::lock_guard<std::mutex> lock(items_mutex_);
//                 for (const auto& item : items_) {
//                     if (item->interval() > 0 && item->interval() < min_interval) {
//                         min_interval = item->interval();
//                     }
//                 }
//             }
            
//             // 等待剩余时间
//             if (elapsed.count() < min_interval) {
//                 std::this_thread::sleep_for(
//                     std::chrono::milliseconds(min_interval - elapsed.count()));
//             }
//         }
//     }

//     void processor_loop() {
//         DataBatch batch;
//         while (running_) {
//             {
//                 std::unique_lock<std::mutex> lock(queue_mutex_);
//                 queue_cv_.wait(lock, [this] {
//                     return !running_ || !data_queue_.empty();
//                 });
//             }
            
//             // 处理队列中的所有数据
//             while (data_queue_.pop(batch)) {
//                 if (batch.empty()) continue;
                
//                 // 写入所有输出插件
//                 std::lock_guard<std::mutex> lock(plugins_mutex_);
//                 for (auto& plugin : plugins_) {
//                     try {
//                         plugin->write(batch);
//                     } catch (const std::exception& e) {
//                         std::cerr << "Error writing to output plugin: " 
//                                   << e.what() << std::endl;
//                     }
//                 }
//             }
            
//             // 定期刷新插件
//             {
//                 std::lock_guard<std::mutex> lock(plugins_mutex_);
//                 for (auto& plugin : plugins_) {
//                     plugin->flush();
//                 }
//             }
//         }
//     }

//     LockFreeQueue<DataBatch> data_queue_;
//     std::mutex queue_mutex_;
//     std::condition_variable queue_cv_;
//     std::atomic<bool> running_;
    
//     std::vector<std::shared_ptr<MonitorItem>> items_;
//     std::mutex items_mutex_;
    
//     std::vector<std::shared_ptr<OutputPlugin>> plugins_;
//     std::mutex plugins_mutex_;
    
//     std::thread collector_thread_;
//     std::thread processor_thread_;
// };

// // 自定义业务指标监控
// class BusinessMonitor : public MonitorItem
// {
// public:
//     using ValueFunc = std::function<double()>;
    
//     BusinessMonitor(const std::string& name, ValueFunc func, 
//                    const std::string& unit = "", int interval_ms = 1000)
//         : name_(name), value_func_(func), unit_(unit), interval_ms_(interval_ms) {}

//     std::string name() const override { return name_; }

//     int interval() const override { return interval_ms_; }

//     DataBatch collect() override {
//         DataBatch batch;
//         try {
//             double value = value_func_();
//             batch.emplace_back(name_, value, unit_);
//         } catch (const std::exception& e) {
//             std::cerr << "Error collecting business metric: " << e.what() << std::endl;
//         }
//         return batch;
//     }

// private:
//     std::string name_;
//     ValueFunc value_func_;
//     std::string unit_;
//     int interval_ms_;
// };

// // ============================== 输出插件实现 ==============================

// // 控制台输出插件
// class ConsoleOutput : public OutputPlugin
// {
// public:
//     void write(const DataBatch& batch) override
//     {
//         for (const auto& point : batch) {
//             std::tm tm = *std::localtime(&point.timestamp);
            
//             std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ["
//                       << point.metric << "] = " << point.value;
            
//             if (!point.unit.empty()) {
//                 std::cout << " " << point.unit;
//             }
            
//             if (!point.tags.empty()) {
//                 std::cout << " {";
//                 for (auto it = point.tags.begin(); it != point.tags.end(); ) {
//                     std::cout << it->first << "=" << it->second;
//                     if (++it != point.tags.end()) {
//                         std::cout << ", ";
//                     }
//                 }
//                 std::cout << "}";
//             }
            
//             std::cout << std::endl;
//         }
//     }

//     void flush() override {
//         std::cout.flush();
//     }
// };

// // 文件输出插件 (CSV格式)
// class CsvFileOutput : public OutputPlugin
// {
// public:
//     CsvFileOutput(const std::string& filename, bool append = true) 
//         : filename_(filename), append_(append) {}
    
//     ~CsvFileOutput() {
//         flush();
//     }

//     void write(const DataBatch& batch) override {
//         if (!file_.is_open()) {
//             open_file();
//         }
        
//         for (const auto& point : batch) {
//             std::tm tm = *std::localtime(&point.timestamp);
            
//             file_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ","
//                   << "\"" << point.metric << "\","
//                   << point.value;
            
//             if (!point.unit.empty()) {
//                 file_ << ",\"" << point.unit << "\"";
//             } else {
//                 file_ << ",\"\"";
//             }
            
//             if (!point.tags.empty()) {
//                 file_ << ",\"";
//                 for (auto it = point.tags.begin(); it != point.tags.end(); ) {
//                     file_ << it->first << "=" << it->second;
//                     if (++it != point.tags.end()) {
//                         file_ << ";";
//                     }
//                 }
//                 file_ << "\"";
//             } else {
//                 file_ << ",\"\"";
//             }
            
//             file_ << "\n";
//         }
//     }

//     void flush() override {
//         if (file_.is_open()) {
//             file_.flush();
//         }
//     }

// private:
//     void open_file() {
//         if (append_) {
//             file_.open(filename_, std::ios::app);
//         } else {
//             file_.open(filename_);
//         }
        
//         if (!file_.is_open()) {
//             throw std::runtime_error("Failed to open file: " + filename_);
//         }
        
//         // 写CSV头
//         if (file_.tellp() == 0) {
//             file_ << "Timestamp,Metric,Value,Unit,Tags\n";
//         }
//     }

//     std::string filename_;
//     bool append_;
//     std::ofstream file_;
// };

// // InfluxDB输出插件
// class InfluxDbOutput : public OutputPlugin {
// public:
//     InfluxDbOutput(const std::string& url, const std::string& database, 
//                   const std::string& username = "", const std::string& password = "")
//         : url_(url), database_(database), username_(username), password_(password) {}
    
//     void write(const DataBatch& batch) override {
//         if (batch.empty()) return;
        
//         std::ostringstream payload;
        
//         for (const auto& point : batch) {
//             // 转义特殊字符
//             std::string metric = escape_key(point.metric);
            
//             // 添加标签
//             std::string tags;
//             for (const auto& tag : point.tags) {
//                 tags += "," + escape_key(tag.first) + "=" + escape_key(tag.second);
//             }
            
//             // 纳秒时间戳            
//             payload << metric << tags << " value=" << point.value << " " << point.timestamp << "\n";
//         }
        
//         // 实际应用中这里会发送HTTP请求
//         std::cout << "[InfluxDB] Writing " << batch.size() << " data points to database: " 
//                   << database_ << "\n";
//         // std::cout << payload.str() << std::endl;
//     }

//     void flush() override {
//         // 在实际实现中，这里会确保所有数据已发送
//     }

// private:
//     static std::string escape_key(const std::string& key) {
//         std::string escaped;
//         for (char c : key) {
//             if (c == ',' || c == '=' || c == ' ') {
//                 escaped += '\\';
//             }
//             escaped += c;
//         }
//         return escaped;
//     }

//     std::string url_;
//     std::string database_;
//     std::string username_;
//     std::string password_;
// };

} // bee