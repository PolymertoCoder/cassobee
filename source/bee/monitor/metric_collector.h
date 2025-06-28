#pragma once
#include <functional>
#include "monitor.h"
#include "types.h"
#include "metric.h"

namespace bee
{

// 收集器基类
class metric_collector
{
public:
    metric_collector(const std::string& name)
        : _name(name)
    {
        monitor_engine::get_instance()->register_collector(this);
    }

    metric_collector(const std::string& name, monitor_engine* pmonitor_engine)
        : _name(name), _monitor_engine(pmonitor_engine)
    {
        ASSERT(pmonitor_engine);
        pmonitor_engine->register_collector(this);
    }

    virtual ~metric_collector() = default;

    // 收集数据到指标
    virtual void collect(metric& metric) final
    {
        if(auto* influx_metric = dynamic_cast<bee::influx_metric*>(&metric))
        {
            collect_impl(*influx_metric);
        }
        else if(auto* prom_metric = dynamic_cast<bee::prometheus_metric*>(&metric))
        {
            collect_impl(*prom_metric);
        }
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
    monitor_engine* _monitor_engine = nullptr;
    std::map<std::string, std::function<void()>> _event_handlers;
};

} // namespace bee