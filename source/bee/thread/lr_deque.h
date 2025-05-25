#pragma once
#include <deque>
#include "left_right.h"

namespace bee {

template<typename T, typename Allocator = std::allocator<T>>
class lr_deque
{
public:
    using value_type = T;
    using size_type = typename std::deque<T, Allocator>::size_type;
    using reference = typename std::deque<T, Allocator>::reference;
    using const_reference = typename std::deque<T, Allocator>::const_reference;

    // 构造/析构
    lr_deque() = default;
    
    explicit lr_deque(const Allocator& alloc) : _lr(alloc) {}

    lr_deque(size_type count, const T& value, const Allocator& alloc = Allocator())
        : _lr(count, value, alloc) {}

    explicit lr_deque(size_type count, const Allocator& alloc = Allocator())
        : _lr(count, alloc) {}

    template <typename InputIt>
    lr_deque(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : _lr(first, last, alloc) {}

    // 容量操作
    bool empty() const
    {
        return _lr.apply_read([](const auto* dq) { return dq->empty(); });
    }

    size_type size() const
    {
        return _lr.apply_read([](const auto* dq) { return dq->size(); });
    }

    // 元素访问
    reference operator[](size_type pos)
    {
        return _lr.apply_read([pos](auto* dq) -> reference { return (*dq)[pos]; });
    }

    const_reference operator[](size_type pos) const
    {
        return _lr.apply_read([pos](const auto* dq) -> const_reference { return (*dq)[pos]; });
    }

    reference front()
    {
        return _lr.apply_read([](auto* dq) -> reference { return dq->front(); });
    }

    const_reference front() const
    {
        return _lr.apply_read([](const auto* dq) -> const_reference { return dq->front(); });
    }

    reference back()
    {
        return _lr.apply_read([](auto* dq) -> reference { return dq->back(); });
    }

    const_reference back() const
    {
        return _lr.apply_read([](const auto* dq) -> const_reference { return dq->back(); });
    }

    // 修改操作
    void push_front(const T& value)
    {
        _lr.apply_write([&](auto* dq) { dq->push_front(value); });
    }

    void push_front(T&& value)
    {
        _lr.apply_write([&](auto* dq) { dq->push_front(std::move(value)); });
    }

    void push_back(const T& value)
    {
        _lr.apply_write([&](auto* dq) { dq->push_back(value); });
    }

    void push_back(T&& value)
    {
        _lr.apply_write([&](auto* dq) { dq->push_back(std::move(value)); });
    }

    template <typename... Args>
    reference emplace_front(Args&&... args)
    {
        return _lr.apply_write([&](auto* dq) -> reference {
            return dq->emplace_front(std::forward<Args>(args)...);
        });
    }

    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
        return _lr.apply_write([&](auto* dq) -> reference {
            return dq->emplace_back(std::forward<Args>(args)...);
        });
    }

    void pop_front()
    {
        _lr.apply_write([](auto* dq) { dq->pop_front(); });
    }

    void pop_back()
    {
        _lr.apply_write([](auto* dq) { dq->pop_back(); });
    }

    void clear()
    {
        _lr.apply_write([](auto* dq) { dq->clear(); });
    }

    // 迭代器
    auto begin()
    {
        return _lr.apply_read([](auto* dq) { return dq->begin(); });
    }

    auto end()
    {
        return _lr.apply_read([](auto* dq) { return dq->end(); });
    }

    auto cbegin() const
    {
        return _lr.apply_read([](const auto* dq) { return dq->cbegin(); });
    }

    auto cend() const
    {
        return _lr.apply_read([](const auto* dq) { return dq->cend(); });
    }

private:
    left_right<std::deque<T, Allocator>> _lr;
};

} // namespace bee