#pragma once
#include "metric_collector.h"
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>
#include <string>

namespace bee
{

class mem_collector : public metric_collector
{
public:
    mem_collector() : metric_collector("memory") 
    {
        set_interval(3000); // 3秒采集一次
    }
    
protected:
    virtual void collect_impl(influx_metric& metric) override
    {
        // 获取系统内存信息
        struct sysinfo mem_info;
        if(sysinfo(&mem_info) != 0)
        {
            return;
        }
        
        // 计算内存大小（考虑mem_unit）
        uint64_t total_ram = mem_info.totalram * mem_info.mem_unit;
        uint64_t free_ram = mem_info.freeram * mem_info.mem_unit;
        uint64_t used_ram = total_ram - free_ram;
        double ram_usage = (used_ram * 100.0) / total_ram;
        
        // 添加物理内存指标
        metric.add_tag("type", "physical");
        metric.add_field("total", total_ram);
        metric.add_field("used", used_ram);
        metric.add_field("free", free_ram);
        metric.add_field("usage", ram_usage);
        
        // 获取交换空间信息
        uint64_t total_swap = mem_info.totalswap * mem_info.mem_unit;
        uint64_t free_swap = mem_info.freeswap * mem_info.mem_unit;
        uint64_t used_swap = total_swap - free_swap;
        double swap_usage = total_swap > 0 ? (used_swap * 100.0) / total_swap : 0.0;
        
        // 添加交换空间指标
        metric.add_tag("type", "swap");
        metric.add_field("total", total_swap);
        metric.add_field("used", used_swap);
        metric.add_field("free", free_swap);
        metric.add_field("usage", swap_usage);
        
        // 获取详细内存信息（从/proc/meminfo）
        parse_proc_meminfo(metric);
    }
    
private:
    void parse_proc_meminfo(influx_metric& metric)
    {
        std::ifstream meminfo("/proc/meminfo");
        if(!meminfo.is_open()) return;
        
        std::unordered_map<std::string, uint64_t> values;
        std::string line;
        
        while(std::getline(meminfo, line))
        {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            std::string unit;
            
            iss >> key >> value >> unit;
            
            // 移除key中的冒号
            if(!key.empty() && key[key.size()-1] == ':')
            {
                key = key.substr(0, key.size()-1);
            }
            
            // 转换为字节（大多数值以kB为单位）
            if(unit == "kB")
            {
                value *= 1024;
            }
            
            values[key] = value;
        }
        
        // 添加详细内存指标
        metric.add_tag("type", "detailed");
        metric.add_field("MemTotal", values["MemTotal"]);
        metric.add_field("MemFree", values["MemFree"]);
        metric.add_field("MemAvailable", values["MemAvailable"]);
        metric.add_field("Buffers", values["Buffers"]);
        metric.add_field("Cached", values["Cached"]);
        metric.add_field("SwapCached", values["SwapCached"]);
        metric.add_field("Active", values["Active"]);
        metric.add_field("Inactive", values["Inactive"]);
        metric.add_field("SwapTotal", values["SwapTotal"]);
        metric.add_field("SwapFree", values["SwapFree"]);
    }
};
    
} // namespace bee