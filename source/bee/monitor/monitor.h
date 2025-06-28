#pragma once
#include "lock.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace bee
{

class metric_collector;
class metric;
class metric_exporter;

class monitor_engine : public singleton_support<monitor_engine>
{
public:
    monitor_engine() = default;
    ~monitor_engine() = default;

    void start();
    void stop();

    void register_collector(metric_collector* collector);
    void unregister_collector(const std::string& name);
    void unregister_collector(metric_collector* collector);
    metric_collector* find_collector(const std::string& name) const;
    
    void register_metric(metric* metric);
    void register_exporter(metric_exporter* exporter);

    void clear();
    
    // 导出所有指标
    void export_to(metric_exporter* exporter);
    
    // 导出所有指标到所有导出器
    void export_all();
    
    // 收集所有指标
    void collect_all();

protected:
    mutable bee::mutex _locker;
    std::unordered_map<std::string, metric_collector*> _collectors;
    std::vector<metric*> _metrics;
    std::vector<metric_exporter*> _exporters;
};

} // namespace bee