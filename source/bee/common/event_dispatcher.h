#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include "lock.h"
#include "glog.h"

namespace bee
{

enum EVENT_HANDLE_RESULT : int
{
    EVENT_HANDLE_SUCCESS,
    EVENT_HANDLE_INTERRUPT,
    EVENT_HANDLE_FAILED,
    EVENT_HANDLE_NOT_REGISTERED
};

// 事件基类
class event_base
{
public:
    virtual ~event_base() = default;

    // 编译期类型ID生成器
    template <typename T>
    static size_t type_id() noexcept
    {
        static char unique_id;
        return reinterpret_cast<size_t>(&unique_id);
    }

    virtual size_t get_type_id() const noexcept = 0;
};

// 事件处理器接口
class event_handler_interface
{
public:
    virtual ~event_handler_interface() = default;
    virtual int handle_event(const event_base& event) = 0;
};

// 具体事件处理器
template <typename EventType>
class event_handler : public event_handler_interface
{
public:
    using callback = std::function<int(const EventType&)>;

    explicit event_handler(callback cb) : _callback(std::move(cb)) {}

    int handle_event(const event_base& event) override
    {
        if(_callback)
        {
            // 安全的类型转换
            if(event.get_type_id() == event_base::type_id<EventType>())
            {
                return _callback(static_cast<const EventType&>(event));
            }
        }
        return EVENT_HANDLE_NOT_REGISTERED;
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
    void register_handler(std::function<int(const EventType&)> handler)
    {
        const size_t type_id = event_base::type_id<EventType>();
        auto handler_ptr = std::make_unique<event_handler<EventType>>(std::move(handler));

        bee::mutex::scoped l(_locker);
        _handlers[type_id].push_back(std::move(handler_ptr));
    }

    // 派发事件
    void dispatch(const event_base& event)
    {
        const size_t type_id = event.get_type_id();
        std::vector<event_handler_interface*> handlers;

        {
            bee::mutex::scoped l(_locker);
            auto it = _handlers.find(type_id);
            if(it != _handlers.end())
            {
                handlers.reserve(it->second.size());
                for(auto& handler : it->second)
                {
                    handlers.push_back(handler.get());
                }
            }
        }

        for(auto handler : handlers)
        {
            int result = handler->handle_event(event);
            if(result != EVENT_HANDLE_SUCCESS)
            {
                local_log("event handle failed, type: %zu, result: %d", type_id, result);
                if(result == EVENT_HANDLE_INTERRUPT)
                    break;
            }
        }
    }
private:
    std::unordered_map<size_t, std::vector<std::unique_ptr<event_handler_interface>>> _handlers;
    bee::mutex _locker;
};

} // namespace bee