#pragma once
#include <unordered_map>
#include "left_right.h"

namespace bee
{

template<
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>
>
class lr_unordered_map
{
public:
    using key_type = Key;
    using mapped_type = T;
    using size_type = typename std::unordered_map<Key, T, Hash, KeyEqual, Allocator>::size_type;

    // 构造/析构
    lr_unordered_map() = default;
    
    explicit lr_unordered_map(const Allocator& alloc) : _lr(alloc) {}

    template <typename InputIt>
    lr_unordered_map(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : _lr(first, last, alloc) {}

    // 容量操作
    bool empty() const
    {
        return _lr.apply_read([](const auto* map) { return map->empty(); });
    }

    size_type size() const
    {
        return _lr.apply_read([](const auto* map) { return map->size(); });
    }

    // 元素访问
    T operator[](const Key& key)
    {
        return _lr.apply_write([&](auto* map) -> T { return (*map)[key]; });
    }

    T at(const Key& key)
    {
        return _lr.apply_read([&](auto* map) -> T { return map->at(key); });
    }

    T at(const Key& key) const
    {
        return _lr.apply_read([&](const auto* map) -> T { return map->at(key); });
    }

    // 修改操作
    template <typename... Args>
    auto emplace(Args&&... args)
    {
        return _lr.apply_write([&](auto* map) {
            return map->emplace(std::forward<Args>(args)...);
        });
    }

    void erase(const Key& key)
    {
        _lr.apply_write([&](auto* map) { map->erase(key); });
    }

    void clear()
    {
        _lr.apply_write([](auto* map) { map->clear(); });
    }

    // 查找操作
    auto find(const Key& key)
    {
        return _lr.apply_read([&](auto* map) { return map->find(key); });
    }

    bool contains(const Key& key) const
    {
        return _lr.apply_read([&](const auto* map) { return map->contains(key); });
    }

private:
    left_right<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> _lr;
};

} // namespace bee