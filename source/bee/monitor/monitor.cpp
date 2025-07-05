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

monitor_engine::~monitor_engine()
{
    stop();
    clear();
}

void monitor_engine::init()
{
    bee::mutex::scoped l(_locker);

    auto* cfg = bee::config::get_instance();

    std::string exporter_name = cfg->get("monitor", "exporter");
    assert(exporter_name.size());
    if(exporter_name == "influx")
    {
        _exporter = new influx_exporter;
    }
    else if(exporter_name == "prometheus")
    {
        // TODO: prometheus exporter
        // register_exporter(new prometheus_exporter);
    }
    assert(_exporter);

    std::string collector_names = cfg->get("monitor", "collectors");
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
}
    
void monitor_engine::start()
{
    bee::mutex::scoped l(_locker);

    TIMETYPE interval = config::get_instance()->get<TIMETYPE>("monitor", "interval", 1000); // ms
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
    if(_timerid < 0) return;
    del_timer(_timerid);
    _timerid = -1;
    local_log("monitor engine stopped.");
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

// 清空注册中心
void monitor_engine::clear()
{
    bee::mutex::scoped l(_locker);

    for(auto& [_, collector] : _collectors)
    {
        delete collector;
    }
    _collectors.clear();

    for(auto* metric : _metrics)
    {
        delete metric;
    }
    _metrics.clear();

    delete _exporter;
    _exporter = nullptr;
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
    for(auto& metric : _metrics)
    {
        metric->export_to(*_exporter);
        delete metric;
        metric = nullptr;
    }
    _metrics.clear();
}

// 收集所有指标
void monitor_engine::collect_all()
{
    bee::mutex::scoped l(_locker);
    for(auto& [_, collector] : _collectors)
    {
        metric* metric = nullptr;
        switch(_exporter->get_type())
        {
            case metric_exporter::TYPE::INFLUX:
            {
                metric = new influx_metric(collector->name());
                break;
            }
            case metric_exporter::TYPE::PROMETHEUS:
            {
                // TODO: prometheus metric
                break;
            }
        }

        if(metric)
        {
            collector->collect(*metric);
            _metrics.push_back(metric);
        }
    }
}

} // bee