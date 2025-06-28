#pragma once
#include "metric_collector.h"
#include "common.h"
#include "glog.h"
#include <sys/resource.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>

namespace bee
{

class proc_collector : public metric_collector
{
public:
    proc_collector() : metric_collector("process") 
    {
        set_interval(2000); // 2秒采集一次
        _pid = getpid(); // 默认监控当前进程
    }
    
    // 设置要监控的进程ID
    void set_pid(pid_t pid) { _pid = pid; }
    
    // 添加要监控的进程名称
    void add_process_name(const std::string& name)
    {
        _process_names.push_back(name);
    }
    
protected:
    virtual void collect_impl(influx_metric& metric) override
    {
        // 监控当前进程
        collect_process(_pid, metric);
        
        // 监控指定名称的进程
        for(const auto& name : _process_names)
        {
            std::vector<pid_t> pids = find_pids_by_name(name);
            for(pid_t pid : pids)
            {
                collect_process(pid, metric);
            }
        }
    }
    
private:
    void collect_process(pid_t pid, influx_metric& metric)
    {
        // 跳过无效PID
        if(pid <= 0) return;
        
        // 获取进程状态信息
        ProcessStat stat = parse_proc_stat(pid);
        if(stat.pid == 0) return; // 进程不存在
        
        // 获取进程资源使用
        struct rusage usage;
        if(getrusage(RUSAGE_SELF, &usage) != 0)
        {
            return;
        }
        
        // 添加进程标识标签
        metric.add_tag("pid", std::to_string(pid));
        metric.add_tag("name", stat.comm);
        
        // 添加进程状态指标
        metric.add_field("cpu_user", stat.utime);
        metric.add_field("cpu_system", stat.stime);
        metric.add_field("cpu_total", stat.utime + stat.stime);
        metric.add_field("vm_size", stat.vsize);
        metric.add_field("rss", stat.rss * sysconf(_SC_PAGESIZE));
        metric.add_field("threads", stat.num_threads);
        
        // 添加资源使用指标
        metric.add_field("user_cpu_usage", usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0);
        metric.add_field("system_cpu_usage", usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0);
        metric.add_field("max_rss", usage.ru_maxrss);
        metric.add_field("minflt", usage.ru_minflt);
        metric.add_field("majflt", usage.ru_majflt);
        metric.add_field("inblock", usage.ru_inblock);
        metric.add_field("oublock", usage.ru_oublock);
        
        // 获取打开文件数
        metric.add_field("open_files", count_open_files(pid));
    }
    
    // 解析/proc/[pid]/stat文件
    struct ProcessStat
    {
        pid_t pid = 0;
        std::string comm;
        char state;
        pid_t ppid;
        uint64_t utime;
        uint64_t stime;
        long rss;
        unsigned long vsize;
        int num_threads;
    };
    
    ProcessStat parse_proc_stat(pid_t pid)
    {
        ProcessStat stat;
        std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
        if(!stat_file.is_open())
        {
            return stat;
        }
        
        std::string line;
        if(!std::getline(stat_file, line))
        {
            return stat;
        }
        
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        
        while(iss >> token)
        {
            tokens.push_back(token);
        }
        
        // 最少需要52个字段
        if(tokens.size() < 52)
        {
            return stat;
        }
        
        try
        {
            stat.pid = std::stoi(tokens[0]);
            
            // 进程名在括号中
            stat.comm = tokens[1].substr(1, tokens[1].size() - 2);
            
            stat.state = tokens[2][0];
            stat.ppid = std::stoi(tokens[3]);
            stat.utime = std::stoull(tokens[13]);
            stat.stime = std::stoull(tokens[14]);
            stat.rss = std::stol(tokens[23]);
            stat.vsize = std::stoul(tokens[22]);
            stat.num_threads = std::stoi(tokens[19]);
        }
        catch (...)
        {
            local_log("parse /proc/" + std::to_string(pid) + "/stat failed");
        }
        
        return stat;
    }
    
    // 计算进程打开的文件数
    int count_open_files(pid_t pid)
    {
        std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
        DIR* dir = opendir(fd_path.c_str());
        if(!dir) return 0;
        
        int count = 0;
        struct dirent* entry;
        
        while((entry = readdir(dir)) != nullptr)
        {
            if(entry->d_type == DT_LNK)
            {
                count++;
            }
        }
        
        closedir(dir);
        return count;
    }
    
    // 通过进程名查找PID
    
    
    pid_t _pid; // 要监控的进程ID
    std::vector<std::string> _process_names; // 要监控的进程名称
};

} // namespace bee