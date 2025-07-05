#pragma once
#include <algorithm>
#include <functional>
#include <vector>
#include "glog.h"
#include "types.h"

namespace bee
{

enum EVENT_HANDLE_RESULT : int
{
    EVENT_HANDLE_SUCCESS,
    EVENT_HANDLE_INTERRUPT,
    EVENT_HANDLE_FAILED,
    EVENT_HANDLE_NOT_REGISTERED
};

enum EVENT_LISTENER_PRIORITY : int
{
    LOW = 0,
    NORMAL = 100,
    HIGH = 200,
    CRITICAL = 300
};

// 事件基类
class event_base
{
public:
    virtual ~event_base() = default;
    int get_type() const noexcept { return _event_type; }

protected:
    int _event_type;
};

// 事件处理器接口
class event_listener
{
public:
    using HANDLER = std::function<int(const event_base*)>;

    event_listener(int event_type, int priority, HANDLER handler)
        : _event_type(event_type), _priority(priority), _handler(handler) {}

    FORCE_INLINE int handle_event(const event_base* event)
    {
        if(_handler)
        {
            return _handler(event);
        }
        return EVENT_HANDLE_NOT_REGISTERED;
    }

    FORCE_INLINE int get_event_type() const noexcept { return _event_type; }
    FORCE_INLINE int get_priority() const noexcept { return _priority; }

private:
    int16_t _event_type = -1;
    int16_t _priority = NORMAL;
    HANDLER _handler;
};

// 事件派发器
class event_dispatcher
{
public:
    event_dispatcher();
    ~event_dispatcher();

    // 注册事件处理器
    void register_handler(int event_type, std::function<int(const event_base*)> handler, int priority = EVENT_LISTENER_PRIORITY::NORMAL)
    {
        if(_listeners.size() <= event_type)
        {
            _listeners.resize(event_type + 1);
        }

        auto& listeners = _listeners[event_type];
        listeners.emplace_back(new event_listener(event_type, priority, handler));
        std::ranges::sort(listeners, [](const event_listener* l, const event_listener* r) { return l->get_priority() > r->get_priority(); });
    }

    // 派发事件
    void dispatch(const event_base& event)
    {
        int event_type = event.get_type();
        if(event_type < 0 || event_type >= _listeners.size())
        {
            local_log("event_type is invalid or has no handler:%d.", event_type);
            return;
        }

        for(auto& listener : _listeners[event_type])
        {
            if(listener->get_event_type() != event_type) continue;
            if(listener->handle_event(&event) == EVENT_HANDLE_INTERRUPT) break;
        }
    }

private:
    std::vector<std::vector<event_listener*>> _listeners;
};

} // namespace bee