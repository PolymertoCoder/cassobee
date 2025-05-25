#pragma once
#include <set>
#include "left_right.h"

namespace bee
{

template<typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
class lr_set
{
public:
    using key_type = Key;
    using size_type = typename std::set<Key, Compare, Allocator>::size_type;

    // 构造/析构
    lr_set() = default;
    
    explicit lr_set(const Allocator& alloc) : _lr(alloc) {}

    template <typename InputIt>
    lr_set(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : _lr(first, last, alloc) {}

    // 容量操作
    bool empty() const
    {
        return _lr.apply_read([](const auto* set) { return set->empty(); });
    }

    size_type size() const
    {
        return _lr.apply_read([](const auto* set) { return set->size(); });
    }

    // 修改操作
    template <typename... Args>
    auto emplace(Args&&... args)
    {
        return _lr.apply_write([&](auto* set) {
            return set->emplace(std::forward<Args>(args)...);
        });
    }

    void erase(const Key& key)
    {
        _lr.apply_write([&](auto* set) { set->erase(key); });
    }

    void clear()
    {
        _lr.apply_write([](auto* set) { set->clear(); });
    }

    // 查找操作
    auto find(const Key& key)
    {
        return _lr.apply_read([&](auto* set) { return set->find(key); });
    }

    bool contains(const Key& key) const
    {
        return _lr.apply_read([&](const auto* set) { return set->contains(key); });
    }

private:
    left_right<std::set<Key, Compare, Allocator>> _lr;
};

} // namespace bee