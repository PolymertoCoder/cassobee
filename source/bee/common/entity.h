#pragma once
#include "lock.h"
#include <vector>
#include <memory>

#define REGISTER_COMPONENT(entity, T) entity->add_component<T>()
#define GET_COMPONENT(entity, T)      entity->get_component<T>()
#define HAS_COMPONENT(entity, T)      entity->has_component<T>()
#define SCOPED_ENTITY_LOCK(entity)    bee::mutex::scoped l((entity)->_locker)

namespace bee
{

class entity;

class component_base
{
public:
    virtual ~component_base() = default;
    virtual void assign() {}

protected:
    friend class component_composer;
    friend class entity;
    entity* _ent = nullptr;
    uint8_t _idx = 0;
    bool _is_assgined = false;
};

template<typename T>
concept component = std::derived_from<T, component_base>;

class component_composer
{
public:
    component_composer(entity* ent) : _ent(ent) {}

    template<component T>
    void add_component()
    {
        auto index = get_component_index<T>();
        if(index >= _components.size())
        {
            _components.resize(index + 1, nullptr);
        }
        if(_components[index] == nullptr)
        {
            _components[index] = new T;
            _components[index]->_ent = _ent;
            _components[index]->_idx = index;
        }
    }

    template<component T>
    T* get_component()
    {
        auto index = get_component_index<T>();
        if(index < _components.size() && _components[index]->_is_assgined)
        {
            return std::static_pointer_cast<T>(_components[index]);
        }
        return nullptr;
    }

    template<component T>
    bool has_component()
    {
        auto index = get_component_index<T>();
        return index < _components.size() && _components[index]->_is_assgined;
    }

    template<component T, typename... Args>
    void assign(Args&&... args)
    {
        if(auto comp = get_component<T>())
        {
            comp->assign(std::forward<Args...>(args...));
            comp->_is_assgined = true;
        }
    }

private:
    template<component T>
    std::size_t get_component_index()
    {
        static const std::size_t index = _next_index++;
        return index;
    }

    entity* _ent = nullptr;
    std::vector<component_base*> _components;
    static std::size_t _next_index;
};

std::size_t component_composer::_next_index = 0;

class entity
{
public:
    entity() : _composer(this) {}

    template<component T>
    void add_component()
    {
        _composer.add_component<T>();
    }

    template<component T>
    T* get_component()
    {
        return _composer.get_component<T>();
    }

    template<component T>
    bool has_component()
    {
        return _composer.has_component<T>();
    }

    template<component T, typename ...Args>
    void assign(Args&&... args)
    {
        _composer.assign<T>(std::forward<Args...>(args...));
    }

    void lock()
    {
        _locker.lock();
    }

    void unlock()
    {
        _locker.unlock();
    }

public:
    bee::mutex _locker;
    component_composer _composer;
};

} // namespace bee