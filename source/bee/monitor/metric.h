#pragma once
#include <string>
#include <map>
#include <vector>
#include "types.h"
#include "monitor.h"
#include "systemtime.h"

namespace bee
{

// 前向声明
class metric_exporter;

// 监控数据点基类
class metric
{
public:
    metric(const std::string& name)
        : _name(name), _timestamp(systemtime::get_nanoseconds()) {}

    metric(const std::string& name, monitor_engine* pmonitor_engine)
        : _name(name), _timestamp(systemtime::get_nanoseconds()), _monitor_engine(pmonitor_engine)
    {
        ASSERT(pmonitor_engine);
        pmonitor_engine->register_metric(this);
    }

    virtual ~metric() = default;

    // 导出到具体导出器
    virtual void export_to(metric_exporter& exp) const = 0;

    // 获取指标名称
    FORCE_INLINE const std::string& name() const { return _name; }
    
    // 获取时间戳
    FORCE_INLINE TIMETYPE timestamp() const { return _timestamp; }
    
    // 设置时间戳
    FORCE_INLINE void set_timestamp(TIMETYPE ts) { _timestamp = ts; }

protected:
    const std::string _name;
    TIMETYPE _timestamp;
    monitor_engine* _monitor_engine = nullptr;
};

// InfluxDB 格式指标
class influx_metric : public metric
{
public:
    influx_metric(const std::string& name) : metric(name) {}
    influx_metric(const std::string& name, monitor_engine* pmonitor_engine) 
        : metric(name, pmonitor_engine) {}
    
    virtual ~influx_metric() = default;

    // 导出到具体导出器
    virtual void export_to(metric_exporter& exp) const override;

    // 添加标签
    void add_tag(const std::string& key, const std::string& value)
    {
        _tags[key] = value;
    }
    
    // 添加字段
    void add_field(const std::string& key, const std::string& value)
    {
        _fields[key] = value;
    }
    
    void add_field(const std::string& key, double value)
    {
        _fields[key] = std::to_string(value);
    }

    void add_field(const std::string& key, int32_t value)
    {
        _fields[key] = std::to_string(value);
    }

    void add_field(const std::string& key, uint32_t value)
    {
        _fields[key] = std::to_string(value);
    }
    
    void add_field(const std::string& key, int64_t value)
    {
        _fields[key] = std::to_string(value) + "i";
    }

    void add_field(const std::string& key, uint64_t value)
    {
        _fields[key] = std::to_string(value) + "u";
    }
    
    void add_field(const std::string& key, bool value)
    {
        _fields[key] = value ? "true" : "false";
    }

    // 获取测量名称
    const std::string& measurement() const { return _name; }
    
    // 获取标签集
    const std::map<std::string, std::string>& tags() const { return _tags; }
    
    // 获取字段集
    const std::map<std::string, std::string>& fields() const { return _fields; }

protected:
    std::map<std::string, std::string> _tags;
    std::map<std::string, std::string> _fields;
};

// Prometheus 格式指标
class prometheus_metric : public metric
{
public:
    enum Type
    {
        COUNTER,    // 计数器（只增不减）
        GAUGE,      // 仪表盘（可增可减）
        HISTOGRAM,  // 直方图
        SUMMARY,    // 摘要
    };

    prometheus_metric(Type type, const std::string& name)
        : metric(name), _type(type) {}

    prometheus_metric(Type type, const std::string& name, monitor_engine* monitor_engine)
        : metric(name, monitor_engine), _type(type) {}

    // 导出到具体导出器
    virtual void export_to(metric_exporter& exp) const override;

    // 获取指标类型
    Type type() const { return _type; }
    
    // 获取帮助文本
    const std::string& help() const { return _help; }
    
    // 设置帮助文本
    void set_help(const std::string& help) { _help = help; }
    
    // 添加标签
    void add_label(const std::string& key, const std::string& value) {
        _labels[key] = value;
    }
    
    // 设置值
    void set_value(double value) { _value = value; }
    
    // 获取值
    double value() const { return _value; }
    
    // 获取标签集
    const std::map<std::string, std::string>& labels() const { return _labels; }
    
    // 获取标签名向量
    std::vector<std::string> label_names() const
    {
        std::vector<std::string> names;
        for(const auto& kv : _labels)
        {
            names.push_back(kv.first);
        }
        return names;
    }
    
    // 获取标签值向量
    std::vector<std::string> label_values() const
    {
        std::vector<std::string> values;
        for(const auto& kv : _labels)
        {
            values.push_back(kv.second);
        }
        return values;
    }

protected:
    const Type _type;
    std::string _help;
    std::map<std::string, std::string> _labels;
    double _value = 0.0;
};

} // namespace bee