#pragma once
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include "lock.h"

namespace bee
{

// 事件基类
class event_base
{
public:
    virtual ~event_base() = default;
};

// 事件处理器接口
class event_handler_interface
{
public:
    virtual ~event_handler_interface() = default;
    virtual void handle_event(const event_base& event) = 0;
};

// 具体事件处理器
template <typename EventType>
requires std::is_base_of_v<event_base, EventType>
class event_handler : public event_handler_interface
{
public:
    using callback = std::function<void(const EventType&)>;
    
    explicit event_handler(callback cb) : _callback(std::move(cb)) {}
    
    void handle_event(const event_base& event) override
    {
        if(_callback)
        {
            // 安全类型转换
            if(auto concrete_event = dynamic_cast<const EventType*>(&event))
            {
                _callback(*concrete_event);
            }
        }
    }

private:
    callback _callback;
};

// 事件派发器
class event_dispatcher
{
public:
    // 注册事件处理器
    template <typename EventType>
    void register_handler(std::function<void(const EventType&)> handler)
    {
        auto handlerPtr = std::make_shared<event_handler<EventType>>(std::move(handler));
        
        std::lock_guard lock(_mutex);
        _handlers[typeid(EventType)].emplace_back(std::move(handlerPtr));
    }

    // 派发事件
    void dispatch(const event_base& event)
    {
        std::vector<std::shared_ptr<event_handler_interface>> handlers;
        {
            std::lock_guard lock(_mutex);
            if(auto iter = _handlers.find(typeid(event)); iter != _handlers.end())
            {
                handlers = iter->second;
            }
        }
        
        for(auto& handler : handlers)
        {
            handler->handle_event(event);
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<event_handler_interface>>> _handlers;
    bee::mutex _mutex;
};

} // namespace bee