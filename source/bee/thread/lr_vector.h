#pragma once
#include <vector>
#include "left_right.h"

namespace bee
{

template <typename T, typename Allocator = std::allocator<T>>
class lr_vector
{
public:
    using value_type = T;
    using size_type = typename std::vector<T, Allocator>::size_type;
    using reference = typename std::vector<T, Allocator>::reference;
    using const_reference = typename std::vector<T, Allocator>::const_reference;

    // 构造/析构
    lr_vector() = default;
    
    explicit lr_vector(const Allocator& alloc) : _lr(alloc) {}

    lr_vector(size_type count, const T& value, const Allocator& alloc = Allocator())
        : _lr(count, value, alloc) {}

    explicit lr_vector(size_type count, const Allocator& alloc = Allocator())
        : _lr(count, alloc) {}

    template <typename InputIt>
    lr_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : _lr(first, last, alloc) {}

    // 容量操作
    bool empty() const
    {
        return _lr.apply_read([](const auto* vec) { return vec->empty(); });
    }

    size_type size() const
    {
        return _lr.apply_read([](const auto* vec) { return vec->size(); });
    }

    size_type capacity() const
    {
        return _lr.apply_read([](const auto* vec) { return vec->capacity(); });
    }

    // 元素访问
    reference operator[](size_type pos)
    {
        return _lr.apply_read([pos](auto* vec) -> reference { return (*vec)[pos]; });
    }

    const_reference operator[](size_type pos) const
    {
        return _lr.apply_read([pos](const auto* vec) -> const_reference { return (*vec)[pos]; });
    }

    reference front()
    {
        return _lr.apply_read([](auto* vec) -> reference { return vec->front(); });
    }

    const_reference front() const
    {
        return _lr.apply_read([](const auto* vec) -> const_reference { return vec->front(); });
    }

    // 修改操作
    void push_back(const T& value)
    {
        _lr.apply_write([&](auto* vec) { vec->push_back(value); });
    }

    void push_back(T&& value)
    {
        _lr.apply_write([&](auto* vec) { vec->push_back(std::move(value)); });
    }

    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
        return _lr.apply_write([&](auto* vec) -> reference {
            return vec->emplace_back(std::forward<Args>(args)...);
        });
    }

    void pop_back()
    {
        _lr.apply_write([](auto* vec) { vec->pop_back(); });
    }

    void clear()
    {
        _lr.apply_write([](auto* vec) { vec->clear(); });
    }

private:
    left_right<std::vector<T, Allocator>> _lr;
};

} // namespace bee