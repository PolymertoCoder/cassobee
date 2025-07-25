#pragma once
#include <functional>
#include "monitor.h"
#include "systemtime.h"
#include "types.h"
#include "metric.h"

namespace bee
{

// 收集器基类
class metric_collector
{
public:
    metric_collector(const std::string& name)
        : _name(name) {}

    metric_collector(const std::string& name, monitor_engine* pmonitor_engine)
        : _name(name)
    {
        ASSERT(pmonitor_engine);
        pmonitor_engine->register_collector(this);
    }

    virtual ~metric_collector() = default;

    // 收集数据到指标
    void collect(metric& metric)
    {
        auto [millseconds, nanoseconds] = systemtime::get_mill_nano_seconds();
        if(millseconds < _last_collect_time + _interval) return;

        if(auto* influx_metric = dynamic_cast<bee::influx_metric*>(&metric))
        {
            influx_metric->set_timestamp(nanoseconds);
            collect_impl(*influx_metric);
        }
        else if(auto* prom_metric = dynamic_cast<bee::prometheus_metric*>(&metric))
        {
            collect_impl(*prom_metric);
        }
        _last_collect_time = millseconds;
    }
    
    // 获取收集器名称
    FORCE_INLINE const std::string& name() const { return _name; }
    
    // 获取采集间隔
    FORCE_INLINE TIMETYPE interval() const { return _interval; }
    
    // 设置采集间隔
    FORCE_INLINE void set_interval(TIMETYPE interval) { _interval = interval; }
    
    // 注册事件处理函数
    void register_event_handler(const std::string& event_type, std::function<void()> handler)
    {
        _event_handlers[event_type] = handler;
    }
    
    // 处理事件
    void handle_event(const std::string& event_type)
    {
        if(auto iter = _event_handlers.find(event_type); iter != _event_handlers.end())
        {
            iter->second();
        }
    }

protected:
    // 具体收集逻辑由子类实现
    virtual void collect_impl(influx_metric& metric) {}
    virtual void collect_impl(prometheus_metric& metric) {}

protected:
    const std::string _name;
    TIMETYPE _interval = 0;
    TIMETYPE _last_collect_time = 0; // ms
    std::map<std::string, std::function<void()>> _event_handlers;
};

} // namespace bee