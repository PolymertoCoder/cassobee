#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "lock.h"

namespace bee
{

using OBSERVER_TOKENID = size_t;

template <typename... Args>
class subject;

// 观察者令牌基类
class observer_token_base
{
public:
    observer_token_base(OBSERVER_TOKENID id) : _id(id) {}
    virtual ~observer_token_base() = default;

protected:
    OBSERVER_TOKENID _id;
};

// 观察者令牌（用于注销观察者）
template <typename... Args>
class observer_token : public observer_token_base
{
public:
    explicit observer_token(subject<Args...>* subject, OBSERVER_TOKENID id)
        : observer_token_base(id), _subject(subject) {}

    ~observer_token()
    {
        if(_subject)
        {
            _subject->unregister_observer(_id);
        }
    }

    // 禁止复制，允许移动
    observer_token(const observer_token&) = delete;
    observer_token& operator=(const observer_token&) = delete;

    observer_token(observer_token&& other) noexcept
        : observer_token_base(other._id), _subject(other._subject)
    {
        other._subject = nullptr;
    }

    observer_token& operator=(observer_token&& other) noexcept
    {
        if(this != &other)
        {
            _subject = other._subject;
            _id = other._id;
            other._subject = nullptr;
        }
        return *this;
    }

private:
    subject<Args...>* _subject;
};

// 主题类（被观察者）
template <typename... Args>
class subject
{
public:
    using Callback = std::function<void(Args...)>;
    using Token = std::unique_ptr<observer_token<Args...>>;

    // 注册观察者（返回可用于注销的令牌）
    Token register_observer(Callback callback)
    {
        bee::mutex::scoped l(_locker);
        const OBSERVER_TOKENID id = _next_id++;
        _callbacks.emplace(id, std::move(callback));
        return std::make_unique<observer_token<Args...>>(this, id);
    }

    // 注销观察者
    void unregister_observer(OBSERVER_TOKENID id)
    {
        bee::mutex::scoped l(_locker);
        _callbacks.erase(id);
    }

    // 通知所有观察者（线程安全高效版）
    void notify(Args... args)
    {
        // 快速复制回调列表（无锁读取）
        std::vector<Callback> current_callbacks;
        {
            bee::mutex::scoped l(_locker);
            current_callbacks.reserve(_callbacks.size());
            for(auto& kv : _callbacks)
            {
                current_callbacks.push_back(kv.second);
            }
        }

        // 无锁执行回调
        for(auto& cb : current_callbacks)
        {
            if(cb)
            {
                cb(args...);
            }
        }
    }

    // 获取观察者数量（用于调试/监控）
    size_t observer_count() const
    {
        bee::mutex::scoped l(_locker);
        return _callbacks.size();
    }

private:
    mutable bee::mutex _locker;
    std::unordered_map<OBSERVER_TOKENID, Callback> _callbacks;
    OBSERVER_TOKENID _next_id = 0;
};

} // namespace bee