#pragma once
#include <cstddef>
#include <assert.h>
#include <memory.h>
#include <cstring>
#include <utility>

#ifdef _REENTRANT
#include "lock.h"
#endif

static constexpr size_t OBJECT_POOL_MAX = 4194304;

/*
 * 1.对象池的大小要合适，太大了浪费，太小了性能提升不明显；
 * 2.并不是所用情况都适合用对象池，对于小对象意义不大；
 */
template<typename T>
class objectpool
{
public:
    typedef struct object_node
    {
        T* _obj = nullptr;
        object_node* _next = nullptr;
    } node_type;

public:
    objectpool() = default;
    objectpool(size_t capacity) { init(capacity); }
    
    void init(size_t capacity)
    {
#ifdef _REENTRANT
        cassobee::spinlock::scoped l(_locker);
#endif
        if(_pool) return;
        assert(capacity <=  OBJECT_POOL_MAX);
        _pool = new node_type[capacity];
        assert(_pool);
        node_type* node = _pool;
        for(size_t i = 0; i < capacity - 1; ++i)
        {
            node->_next = node + 1;
            node = node->_next;
        }
        node->_next = nullptr;
        _free = _pool;
        _size = 0;
        _cap = capacity;
    }

    ~objectpool()
    {
        destory();
    }

    std::pair<int, T*> alloc()
    {
#ifdef _REENTRANT
        cassobee::spinlock::scoped l(_locker);
#endif
        if(_size == _cap) return {-1, nullptr};
        node_type* node = _free;
        _free = node->_next;
        if(node->_obj == nullptr)
        {
            node->_obj = new T;
        }
        ++_size;
        return {node - _pool, node->_obj};
    }

    void free(size_t objidx)
    {
#ifdef _REENTRANT
        cassobee::spinlock::scoped l(_locker);
#endif
        node_type* node = _pool + objidx;
        if(node < _pool || node >= _pool + _cap) return;
        node->_next = _free;
        _free = node;
        --_size;
    }

    T* find_object(size_t objidx)
    {
#ifdef _REENTRANT
        cassobee::spinlock::scoped l(_locker);
#endif
        if(objidx > _cap) return nullptr;
        return (_pool + objidx)->_obj;
    }

    void destory()
    {
#ifdef _REENTRANT
        cassobee::spinlock::scoped l(_locker);
#endif
        if(!_pool) return;
        node_type* node = _pool;
        for(size_t i = 0; i < _cap; ++i)
        {
            if(node->_obj)
            {
                delete node->_obj;
                node->_obj = nullptr;
            }
            node = node->_next;
        }
        delete[] _pool;
        _pool = nullptr;
        _free = nullptr;
        _size = 0;
        _cap = 0;
    }

private:
#ifdef _REENTRANT
    cassobee::spinlock _locker;
#endif
    node_type* _pool = nullptr;
    node_type* _free = nullptr;
    size_t _size = 0;
    size_t _cap = 0;
};
