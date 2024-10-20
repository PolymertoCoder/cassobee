#pragma once
#include <map>
#include "left_right.h"

template<typename key_type, typename value_type>
class lr_map
{
public:
    using iterator = std::map<key_type, value_type>::iterator;
    using const_iterator = std::map<key_type, value_type>::const_iterator;

    template<typename ...Args>
    void emplace(Args&&... args)
    {
        _lr.apply_write([&](auto* map) { map->emplace(std::forward<Args>(args)...); });
    }

    void erase(const key_type& key)
    {
        _lr.apply_write([&](auto* map) { map->erase(key); });
    }

    void erase(const iterator& iter)
    {
        _lr.apply_write([&](auto* map) { map->erase(iter); });
    }

    void clear()
    {
        _lr.apply_write([&](auto* map) { map->clear(); });
    }

    auto find(const key_type& key)
    {
        return _lr.apply_read([&](auto* map) { return map->find(key); });
    }

    auto operator[](const key_type& key)
    {
        return _lr.apply_read([&](auto* map) { return (*map)[key]; });
    }

    bool contains(const key_type& key)
    {
        return _lr.apply_read([&](auto* map) { return map->contains(key); });
    }

    auto count(const key_type& key)
    {
        return _lr.apply_read([&](auto* map) { return map->count(key); });
    }

    bool empty()
    {
        return _lr.apply_read([&](auto* map) { return map->empty(); });
    }

private:
    left_right<std::map<key_type, value_type>> _lr;
};