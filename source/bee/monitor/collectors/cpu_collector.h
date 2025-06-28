#pragma once
#include "metric_collector.h"
#include "metric.h"
#include <fstream>
#include <sstream>

namespace bee
{

class cpu_collector : public metric_collector
{
public:
    cpu_collector() : metric_collector("cpu")
    {
        set_interval(1000); // 1秒采集一次
    }

protected:
    virtual void collect_impl(influx_metric& metric) override
    {
        if(auto* influx_metric = dynamic_cast<bee::influx_metric*>(&metric))
        {
            int64_t total = 0;
            double idle = 0.0, usage = 0.0;

            std::ifstream file("/proc/stat");
            std::string line;
            if(std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string cpu;
                int64_t user, nice, system, iowait, irq, softirq;
                iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

                total = user + nice + system + idle + iowait + irq + softirq;

                if(total > _last_total && idle > _last_idle)
                {
                    double total_diff = static_cast<double>(total - _last_total);
                    double idle_diff = static_cast<double>(idle - _last_idle);
                    usage = 100.0 * (1.0 - idle_diff / total_diff);
                }
                
                _last_total = total;
                _last_idle = idle;
            }

            // 填充指标
            // influx_metric->add_tag("host", get_host_name());
            influx_metric->add_tag("core", "all");
            influx_metric->add_field("total", total);
            influx_metric->add_field("idle", idle);
            influx_metric->add_field("usage", usage);
        }
    }
    
private:
    int64_t _last_total = 0;
    int64_t _last_idle = 0;
};
    
} // namespace bee