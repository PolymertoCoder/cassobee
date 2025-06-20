#pragma once
#include "factory.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <algorithm>
#include <functional>
#include <memory>

namespace bee
{

enum LOAD_BALANCE_STRATEGY
{
    ROUND_ROBIN,
    RANDOM,
    LEAST_CONNECTIONS,
};

// 负载均衡策略接口
template <typename T>
class load_balance_strategy
{
public:
    virtual ~load_balance_strategy() = default;
    virtual T select(const std::vector<T>& resources) = 0;
    virtual void update(const std::vector<T>& resources) {} // 资源更新通知
};

// 轮询策略
template <typename T>
class round_robin_strategy : public load_balance_strategy<T>
{
public:
    round_robin_strategy() : _index(0) {}
    
    T select(const std::vector<T>& resources) override
    {
        if(resources.empty())
        {
            throw std::runtime_error("No resources available");
        }
        size_t idx = _index++ % resources.size();
        return resources[idx];
    }

    void update(const std::vector<T>&) override
    {
        _index = 0;  // 重置索引
    }

private:
    std::atomic<size_t> _index;
};

// 随机策略
template <typename T>
class random_strategy : public load_balance_strategy<T>
{
public:
    T select(const std::vector<T>& resources) override
    {
        if(resources.empty())
        {
            throw std::runtime_error("No resources available");
        }
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, resources.size() - 1);
        return resources[dist(rng)];
    }
};

// 最少连接策略（需要资源支持连接计数）
template <typename T>
class least_connections_strategy : public load_balance_strategy<T>
{
public:
    using ConnectionCounter = std::function<size_t(const T&)>;
    
    least_connections_strategy(ConnectionCounter counter) 
        : connection_counter_(counter) {}
    
    T select(const std::vector<T>& resources) override
    {
        if(resources.empty())
        {
            throw std::runtime_error("No resources available");
        }
        
        auto it = std::min_element(resources.begin(), resources.end(),
            [&](const T& a, const T& b) {
                return connection_counter_(a) < connection_counter_(b);
            });
        
        return *it;
    }

private:
    ConnectionCounter connection_counter_;
};

// 主负载均衡器
template <typename T>
class load_balancer
{
public:
    // 使用默认策略（轮询）
    load_balancer() : _strategy(std::make_unique<round_robin_strategy<T>>()) {}
    
    // 使用自定义策略
    explicit load_balancer(std::unique_ptr<load_balance_strategy<T>> strategy)
        : _strategy(std::move(strategy)) {}
    
    // 添加资源
    void add_resource(const T& resource)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _resources.push_back(resource);
        notify_strategy_update();
    }
    
    // 添加多个资源
    void add_resources(const std::vector<T>& resources)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for(const auto& res : resources)
        {
            _resources.push_back(res);
        }
        notify_strategy_update();
    }
    
    // 移除资源
    bool del_resource(const T& resource)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::find(_resources.begin(), _resources.end(), resource);
        if(it != _resources.end())
        {
            _resources.erase(it);
            notify_strategy_update();
            return true;
        }
        return false;
    }
    
    // 获取下一个资源
    T get_nect()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _strategy->select(_resources);
    }
    
    // 设置新策略
    void set_strategy(std::unique_ptr<load_balance_strategy<T>> new_strategy)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _strategy = std::move(new_strategy);
        notify_strategy_update();
    }
    
    // 获取当前资源列表
    std::vector<T> get_resources() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _resources;
    }

private:
    void notify_strategy_update()
    {
        _strategy->update(_resources);
    }
    
    std::vector<T> _resources;
    std::unique_ptr<load_balance_strategy<T>> _strategy;
    mutable std::mutex _mutex;
};

template<typename T>
using load_balance_strategy_factory = factory_template<load_balance_strategy<T>,
                                                       round_robin_strategy<T>,
                                                       random_strategy<T>,
                                                       least_connections_strategy<T>>;

} // namespace bee