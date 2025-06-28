#pragma once
#include "metric_collector.h"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

namespace bee
{

class net_collector : public metric_collector
{
public:
    net_collector() : metric_collector("network") 
    {
        set_interval(2000); // 2秒采集一次
        init_network_interfaces();
    }
    
    // 添加要监控的接口（默认监控所有接口）
    void add_interface(const std::string& iface)
    {
        if(std::ranges::find(_interfaces, iface) == _interfaces.end())
        {
            _interfaces.push_back(iface);
        }
    }
    
    // 设置监控TCP连接统计
    void set_monitor_tcp(bool enable) { _monitor_tcp = enable; }
    
    // 设置监控UDP连接统计
    void set_monitor_udp(bool enable) { _monitor_udp = enable; }
    
    // 设置监控连接状态
    void set_monitor_connections(bool enable) { _monitor_connections = enable; }
    
protected:
    virtual void collect_impl(influx_metric& metric) override
    {
        // 获取网络接口统计
        collect_interface_stats(metric);
        
        // 获取TCP/UDP统计
        if(_monitor_tcp)
        {
            collect_tcp_stats(metric);
        }
        if(_monitor_udp)
        {
            collect_udp_stats(metric);
        }
        
        // 获取连接状态
        if(_monitor_connections)
        {
            collect_connection_stats(metric);
        }
    }
    
private:
    // 初始化网络接口列表
    void init_network_interfaces()
    {
        struct ifaddrs *ifaddr, *ifa;
        if(getifaddrs(&ifaddr) == -1)
        {
            return;
        }
        
        for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if(ifa->ifa_addr == nullptr) continue;
            
            // 只关注IPv4接口
            if(ifa->ifa_addr->sa_family == AF_INET)
            {
                std::string iface_name(ifa->ifa_name);
                if(_interfaces_set.find(iface_name) == _interfaces_set.end())
                {
                    _interfaces.push_back(iface_name);
                    _interfaces_set.insert(iface_name);
                }
            }
        }
        
        freeifaddrs(ifaddr);
    }
    
    // 收集网络接口统计信息
    void collect_interface_stats(influx_metric& metric)
    {
        std::ifstream netdev("/proc/net/dev");
        if(!netdev.is_open()) return;
        
        std::string line;
        // 跳过前两行标题
        std::getline(netdev, line);
        std::getline(netdev, line);
        
        while(std::getline(netdev, line))
        {
            std::istringstream iss(line);
            std::string iface;
            iss >> iface;
            
            // 去除接口名后的冒号
            if(iface.back() == ':')
            {
                iface.pop_back();
            }
            
            // 如果指定了特定接口，跳过未指定的
            if(!_interfaces.empty() && std::find(_interfaces.begin(), _interfaces.end(), iface) == _interfaces.end())
            {
                continue;
            }
            
            // 读取接口统计信息
            uint64_t rx_bytes, rx_packets, rx_errors, rx_dropped;
            uint64_t tx_bytes, tx_packets, tx_errors, tx_dropped;
            
            iss >> rx_bytes >> rx_packets >> rx_errors >> rx_dropped
                >> rx_dropped >> rx_dropped >> rx_dropped >> rx_dropped // 跳过一些字段
                >> tx_bytes >> tx_packets >> tx_errors >> tx_dropped;
            
            // 计算增量（与上次采集的差值）
            auto& prev = _prev_stats[iface];
            uint64_t rx_bytes_delta = rx_bytes - prev.rx_bytes;
            uint64_t tx_bytes_delta = tx_bytes - prev.tx_bytes;
            uint64_t rx_packets_delta = rx_packets - prev.rx_packets;
            uint64_t tx_packets_delta = tx_packets - prev.tx_packets;
            uint64_t rx_errors_delta = rx_errors - prev.rx_errors;
            uint64_t tx_errors_delta = tx_errors - prev.tx_errors;
            uint64_t rx_dropped_delta = rx_dropped - prev.rx_dropped;
            uint64_t tx_dropped_delta = tx_dropped - prev.tx_dropped;
            
            // 保存当前状态供下次比较
            prev = {rx_bytes, rx_packets, rx_errors, rx_dropped,
                    tx_bytes, tx_packets, tx_errors, tx_dropped};
            
            // 添加接口指标
            metric.add_tag("interface", iface);
            metric.add_field("rx_bytes", rx_bytes_delta);
            metric.add_field("tx_bytes", tx_bytes_delta);
            metric.add_field("rx_packets", rx_packets_delta);
            metric.add_field("tx_packets", tx_packets_delta);
            metric.add_field("rx_errors", rx_errors_delta);
            metric.add_field("tx_errors", tx_errors_delta);
            metric.add_field("rx_dropped", rx_dropped_delta);
            metric.add_field("tx_dropped", tx_dropped_delta);
            
            // 获取接口速度
            int speed = get_interface_speed(iface);
            if(speed > 0)
            {
                metric.add_field("speed", speed);
            }
            
            // 获取IP地址
            std::string ip = get_interface_ip(iface);
            if(!ip.empty())
            {
                metric.add_tag("ip", ip);
            }
        }
    }
    
    // 获取接口速度（Mbps）
    int get_interface_speed(const std::string& iface)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0) return -1;
        
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ-1);
        
        struct ethtool_cmd ecmd;
        ecmd.cmd = ETHTOOL_GSET;
        ifr.ifr_data = (caddr_t)&ecmd;
        
        if(ioctl(sock, SIOCETHTOOL, &ifr) == -1)
        {
            close(sock);
            return -1;
        }
        
        close(sock);
        return ecmd.speed;
    }
    
    // 获取接口IP地址
    std::string get_interface_ip(const std::string& iface)
    {
        struct ifaddrs *ifaddr, *ifa;
        if(getifaddrs(&ifaddr) == -1)
        {
            return "";
        }
        
        std::string ip;
        for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if(ifa->ifa_addr == nullptr || strcmp(ifa->ifa_name, iface.c_str()) != 0)
            {
                continue;
            }
            
            if(ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa->sin_addr), ip_str, INET_ADDRSTRLEN);
                ip = ip_str;
                break;
            }
        }
        
        freeifaddrs(ifaddr);
        return ip;
    }
    
    // 收集TCP统计信息
    void collect_tcp_stats(influx_metric& metric)
    {
        std::ifstream tcp_snmp("/proc/net/snmp");
        if(!tcp_snmp.is_open()) return;
        
        std::string line;
        bool found_tcp = false;
        
        while(std::getline(tcp_snmp, line))
        {
            if(line.find("Tcp:") == 0)
            {
                found_tcp = true;
                
                // 跳过标题行
                std::getline(tcp_snmp, line);
                break;
            }
        }
        
        if(!found_tcp) return;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix; // 跳过"Tcp:"
        
        uint64_t active_opens, passive_opens, attempts, estab, in_seg, out_seg;
        uint64_t retrans, in_err, out_rst;
        
        iss >> active_opens >> passive_opens >> attempts >> estab
            >> in_seg >> out_seg >> retrans >> in_err >> out_rst;
        
        // 添加TCP指标
        metric.add_tag("protocol", "tcp");
        metric.add_field("active_opens", active_opens);
        metric.add_field("passive_opens", passive_opens);
        metric.add_field("attempts", attempts);
        metric.add_field("established", estab);
        metric.add_field("in_segments", in_seg);
        metric.add_field("out_segments", out_seg);
        metric.add_field("retrans_segments", retrans);
        metric.add_field("in_errors", in_err);
        metric.add_field("out_resets", out_rst);
    }
    
    // 收集UDP统计信息
    void collect_udp_stats(influx_metric& metric)
    {
        std::ifstream udp_snmp("/proc/net/snmp");
        if(!udp_snmp.is_open()) return;
        
        std::string line;
        bool found_udp = false;
        
        while(std::getline(udp_snmp, line))
        {
            if(line.find("Udp:") == 0)
            {
                found_udp = true;
                
                // 跳过标题行
                std::getline(udp_snmp, line);
                break;
            }
        }
        
        if(!found_udp) return;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix; // 跳过"Udp:"
        
        uint64_t in_datagrams, out_datagrams, no_ports, in_errors;
        
        iss >> in_datagrams >> no_ports >> in_errors >> out_datagrams;
        
        // 添加UDP指标
        metric.add_tag("protocol", "udp");
        metric.add_field("in_datagrams", in_datagrams);
        metric.add_field("out_datagrams", out_datagrams);
        metric.add_field("no_ports", no_ports);
        metric.add_field("in_errors", in_errors);
    }
    
    // 收集连接状态信息
    void collect_connection_stats(influx_metric& metric)
    {
        std::ifstream tcp("/proc/net/tcp");
        std::ifstream udp("/proc/net/udp");
        std::ifstream tcp6("/proc/net/tcp6");
        std::ifstream udp6("/proc/net/udp6");
        
        std::unordered_map<std::string, int> state_count;
        std::unordered_map<std::string, int> remote_count;
        
        // 处理TCP连接
        if(tcp.is_open())
        {
            process_connections(tcp, state_count, remote_count);
        }
        if(tcp6.is_open())
        {
            process_connections(tcp6, state_count, remote_count);
        }
        
        // 处理UDP连接
        if(udp.is_open())
        {
            process_connections(udp, state_count, remote_count);
        }
        if(udp6.is_open())
        {
            process_connections(udp6, state_count, remote_count);
        }
        
        // 添加连接状态指标
        for(const auto& kv : state_count)
        {
            metric.add_tag("state", kv.first);
            metric.add_field("connections", kv.second);
        }
        
        // 添加远程连接指标（前10个）
        int count = 0;
        for(const auto& kv : remote_count)
        {
            if(count++ >= 10) break; // 限制数量
            
            metric.add_tag("remote", kv.first);
            metric.add_field("remote_connections", kv.second);
        }
    }
    
    // 处理连接文件
    void process_connections(std::istream& stream, 
                            std::unordered_map<std::string, int>& state_count,
                            std::unordered_map<std::string, int>& remote_count)
    {
        std::string line;
        // 跳过标题行
        std::getline(stream, line);
        
        while(std::getline(stream, line))
        {
            std::istringstream iss(line);
            std::string sl, local, remote, state_str, txq, rxq, tr, retr, uid, timeout, inode;
            
            iss >> sl >> local >> remote >> state_str >> txq >> rxq >> tr >> retr >> uid >> timeout >> inode;
            
            // 转换状态
            int state = std::stoi(state_str, nullptr, 16);
            std::string state_name = tcp_state_to_string(state);
            
            // 统计状态
            state_count[state_name]++;
            
            // 统计远程连接（只统计ESTABLISHED状态）
            if(state == TCP_ESTABLISHED)
            {
                std::string remote_ip = parse_hex_ip(remote);
                remote_count[remote_ip]++;
            }
        }
    }
    
    // 解析16进制格式的IP地址
    std::string parse_hex_ip(const std::string& hex_str)
    {
        if(hex_str.size() < 8) return "";
        
        // IPv4格式: AABBCCDD:PORT
        if(hex_str.find(':') != std::string::npos)
        {
            uint32_t ip_bin = std::stoul(hex_str.substr(0, 8), nullptr, 16);
            struct in_addr addr;
            addr.s_addr = ntohl(ip_bin); // 转换为网络字节序
            
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
            return ip_str;
        }
        // IPv6格式: 32个16进制字符
        else if(hex_str.size() == 32)
        {
            // 简化为前16个字符（实际IPv6地址解析更复杂，这里简化处理）
            return hex_str.substr(0, 16);
        }
        
        return "";
    }
    
    // TCP状态转换
    std::string tcp_state_to_string(int state)
    {
        switch(state)
        {
            case TCP_ESTABLISHED: return "ESTABLISHED";
            case TCP_SYN_SENT: return "SYN_SENT";
            case TCP_SYN_RECV: return "SYN_RECV";
            case TCP_FIN_WAIT1: return "FIN_WAIT1";
            case TCP_FIN_WAIT2: return "FIN_WAIT2";
            case TCP_TIME_WAIT: return "TIME_WAIT";
            case TCP_CLOSE: return "CLOSE";
            case TCP_CLOSE_WAIT: return "CLOSE_WAIT";
            case TCP_LAST_ACK: return "LAST_ACK";
            case TCP_LISTEN: return "LISTEN";
            case TCP_CLOSING: return "CLOSING";
            default: return "UNKNOWN";
        }
    }
    
    // TCP状态常量
    enum
    {
        TCP_ESTABLISHED = 1,
        TCP_SYN_SENT,
        TCP_SYN_RECV,
        TCP_FIN_WAIT1,
        TCP_FIN_WAIT2,
        TCP_TIME_WAIT,
        TCP_CLOSE,
        TCP_CLOSE_WAIT,
        TCP_LAST_ACK,
        TCP_LISTEN,
        TCP_CLOSING
    };
    
    // 接口统计数据结构
    struct InterfaceStats
    {
        uint64_t rx_bytes = 0;
        uint64_t rx_packets = 0;
        uint64_t rx_errors = 0;
        uint64_t rx_dropped = 0;
        uint64_t tx_bytes = 0;
        uint64_t tx_packets = 0;
        uint64_t tx_errors = 0;
        uint64_t tx_dropped = 0;
    };
    
    std::vector<std::string> _interfaces;      // 要监控的接口列表
    std::unordered_set<std::string> _interfaces_set; // 用于快速查找
    std::unordered_map<std::string, InterfaceStats> _prev_stats;
    
    bool _monitor_tcp = true;
    bool _monitor_udp = true;
    bool _monitor_connections = true;
};

} // namespace bee