#pragma once
#include "metric_collector.h"
#include <sys/statvfs.h>
#include <vector>
#include <unordered_set>
#include <string>
#include <fstream>
#include <sstream>

namespace bee
{

class disk_collector : public metric_collector
{
public:
    disk_collector() : metric_collector("disk") 
    {
        set_interval(5000); // 5秒采集一次
        init_mount_points();
    }

protected:
    virtual void collect_impl(influx_metric& metric) override
    {
        for(const auto& mount : _mount_points)
        {
            struct statvfs vfs;
            if(statvfs(mount.c_str(), &vfs) != 0) continue;
            
            // 计算磁盘使用情况
            uint64_t total = static_cast<uint64_t>(vfs.f_blocks) * vfs.f_frsize;
            uint64_t free = static_cast<uint64_t>(vfs.f_bfree) * vfs.f_frsize;
            uint64_t used = total - free;
            double usage = (used * 100.0) / total;
            
            // 添加磁盘指标
            metric.add_tag("mount", mount);
            metric.add_field("total", total);
            metric.add_field("used", used);
            metric.add_field("free", free);
            metric.add_field("usage", usage);
            
            // 添加inode指标
            metric.add_field("inode_total", vfs.f_files);
            metric.add_field("inode_used", vfs.f_files - vfs.f_ffree);
            metric.add_field("inode_free", vfs.f_ffree);
            metric.add_field("inode_usage", (vfs.f_files - vfs.f_ffree) * 100.0 / vfs.f_files);
        }
    }
    
private:
    void init_mount_points()
    {
        std::ifstream mounts("/proc/mounts");
        if(!mounts.is_open()) return;
        
        std::string line;
        while(std::getline(mounts, line))
        {
            std::istringstream iss(line);
            std::string device, mount_point, fstype;
            iss >> device >> mount_point >> fstype;
            
            // 过滤不需要的文件系统
            if(fstype == "tmpfs" || fstype == "devtmpfs" || 
                fstype == "proc" || fstype == "sysfs" || 
                fstype == "cgroup" || fstype == "devpts") continue;
            
            // 去重
            if(_mount_points_set.find(mount_point) == _mount_points_set.end())
            {
                _mount_points.push_back(mount_point);
                _mount_points_set.insert(mount_point);
            }
        }
    }
    
    std::vector<std::string> _mount_points;
    std::unordered_set<std::string> _mount_points_set;
};
    
} // namespace bee