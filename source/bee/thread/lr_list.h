#pragma once
#include <list>
#include "left_right.h"

namespace bee
{

template<typename T, typename Allocator = std::allocator<T>>
class lr_list
{
public:
    using value_type = T;
    using size_type = typename std::list<T, Allocator>::size_type;
    using reference = typename std::list<T, Allocator>::reference;
    using const_reference = typename std::list<T, Allocator>::const_reference;

    // 构造/析构
    lr_list() = default;
    
    explicit lr_list(const Allocator& alloc) : _lr(alloc) {}

    lr_list(size_type count, const T& value, const Allocator& alloc = Allocator())
        : _lr(count, value, alloc) {}

    explicit lr_list(size_type count, const Allocator& alloc = Allocator())
        : _lr(count, alloc) {}

    template <typename InputIt>
    lr_list(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : _lr(first, last, alloc) {}

    // 容量操作
    bool empty() const
    {
        return _lr.apply_read([](const auto* lst) { return lst->empty(); });
    }

    size_type size() const
    {
        return _lr.apply_read([](const auto* lst) { return lst->size(); });
    }

    // 元素访问
    reference front()
    {
        return _lr.apply_read([](auto* lst) -> reference { return lst->front(); });
    }

    const_reference front() const
    {
        return _lr.apply_read([](const auto* lst) -> const_reference { return lst->front(); });
    }

    reference back()
    {
        return _lr.apply_read([](auto* lst) -> reference { return lst->back(); });
    }

    const_reference back() const
    {
        return _lr.apply_read([](const auto* lst) -> const_reference { return lst->back(); });
    }

    // 修改操作
    void push_front(const T& value)
    {
        _lr.apply_write([&](auto* lst) { lst->push_front(value); });
    }

    void push_front(T&& value)
    {
        _lr.apply_write([&](auto* lst) { lst->push_front(std::move(value)); });
    }

    void push_back(const T& value)
    {
        _lr.apply_write([&](auto* lst) { lst->push_back(value); });
    }

    void push_back(T&& value)
    {
        _lr.apply_write([&](auto* lst) { lst->push_back(std::move(value)); });
    }

    template <typename... Args>
    reference emplace_front(Args&&... args)
    {
        return _lr.apply_write([&](auto* lst) -> reference {
            return lst->emplace_front(std::forward<Args>(args)...);
        });
    }

    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
        return _lr.apply_write([&](auto* lst) -> reference {
            return lst->emplace_back(std::forward<Args>(args)...);
        });
    }

    void pop_front()
    {
        _lr.apply_write([](auto* lst) { lst->pop_front(); });
    }

    void pop_back()
    {
        _lr.apply_write([](auto* lst) { lst->pop_back(); });
    }

    void clear()
    {
        _lr.apply_write([](auto* lst) { lst->clear(); });
    }

    // 迭代器
    auto begin()
    {
        return _lr.apply_read([](auto* lst) { return lst->begin(); });
    }

    auto end()
    {
        return _lr.apply_read([](auto* lst) { return lst->end(); });
    }

    auto cbegin() const
    {
        return _lr.apply_read([](const auto* lst) { return lst->cbegin(); });
    }

    auto cend() const
    {
        return _lr.apply_read([](const auto* lst) { return lst->cend(); });
    }

private:
    left_right<std::list<T, Allocator>> _lr;
};

} // namespace bee