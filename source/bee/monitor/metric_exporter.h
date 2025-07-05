#pragma once
#include "metric.h"
#include "glog.h"
#include "systemtime.h"
#include <vector>
#include <string>

namespace bee
{

class metric_exporter
{
public:
    enum TYPE
    {
        INFLUX,
        PROMETHEUS,
        COUNT
    };

    virtual ~metric_exporter() = default;
    virtual int get_type() const = 0;
    virtual void write(const metric* metric) = 0;
};

class influx_exporter : public metric_exporter
{
public:
    influx_exporter() = default;
    virtual ~influx_exporter() = default;

    virtual int get_type() const override { return TYPE::INFLUX; }

    virtual void write(const metric* metric) override
    {
        const auto* infl_metric = dynamic_cast<const influx_metric*>(metric);
        if(!infl_metric) return;

        logclient::get_instance()->influx_log(
            infl_metric->measurement(), 
            infl_metric->fields(), 
            infl_metric->tags(), 
            systemtime::get_nanoseconds()
        );
    }
};

class prometheus_exporter : public metric_exporter	
{
public:
    prometheus_exporter() = default;
    virtual ~prometheus_exporter() = default;

    // 写入头部信息
    virtual void write_header(const metric& metric) = 0;

    // 写入样本数据（各种类型）
    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             float value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             double value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             uint32_t value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             int32_t value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             uint64_t value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             int64_t value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual void write_sample(const metric& metric, 
                             const std::vector<std::string>& label_names, 
                             const std::vector<std::string>& label_values, 
                             const std::string& value, 
                             TIMETYPE timestamp = 0) = 0;

    virtual int get_type() const override { return TYPE::PROMETHEUS; }

    virtual void write(const metric* metric) override
    {
        const auto* prom_metric = dynamic_cast<const prometheus_metric*>(metric);
        if(!prom_metric) return;
        
        // 写入头部信息
        write_header(*prom_metric);
        
        // 写入样本数据
        write_sample(*prom_metric, 
                    prom_metric->label_names(), 
                    prom_metric->label_values(), 
                    prom_metric->value(),
                    prom_metric->timestamp());
    }
};

} // namespace bee