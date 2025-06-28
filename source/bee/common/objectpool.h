#pragma once
#include <atomic>
#include <cstddef>
#include <assert.h>
#include <cstdlib>
#include <memory.h>
#include <cstring>
#include <utility>
#include <unordered_map>

#ifdef _REENTRANT
#include "lock.h"
#endif

namespace bee
{

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
    objectpool() noexcept = default;
    objectpool(size_t capacity) { init(capacity); }

    void init(size_t capacity)
    {
    #ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
    #endif
        if(_pool) return;
        assert(capacity > 0 && capacity <=  OBJECT_POOL_MAX);
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
        destroy();
    }

    std::pair<int, T*> alloc()
    {
#ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
#endif
        if(!_pool) return {-1, nullptr};
        if(_size == _cap) return {-1, nullptr};
        node_type* node = _free;
        _free = node->_next;
        if(node->_obj == nullptr)
        {
            node->_obj = new T;
            _obj_map[node->_obj] = static_cast<size_t>(node - _pool);
        }
        ++_size;
        return {node - _pool, node->_obj};
    }

    void free(T* obj)
    {
#ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
#endif
        auto iter = _obj_map.find(obj);
        if(iter == _obj_map.end()) return;

        const size_t objidx = iter->second;
        node_type* node = _pool + objidx;
        if(node < _pool || node >= _pool + _cap) return;

        _obj_map.erase(iter);
        node->_next = _free;
        _free = node;
        --_size;
    }

    void free(size_t objidx)
    {
#ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
#endif
        node_type* node = _pool + objidx;
        if(node < _pool || node >= _pool + _cap) return;

        if(node->_obj) _obj_map.erase(node->_obj);
        node->_next = _free;
        _free = node;
        --_size;
    }

    T* find_object(size_t objidx)
    {
#ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
#endif
        if(objidx > _cap) return nullptr;
        return (_pool + objidx)->_obj;
    }

    void destroy()
    {
#ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
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
        _obj_map.clear();
    }

    size_t size() const { return _size; }
    size_t capacity() const { return _cap; }

protected:
    node_type* find_node(size_t objidx)
    {
    #ifdef _REENTRANT
        bee::spinlock::scoped l(_locker);
    #endif
        if(objidx >= _cap) return nullptr;
        return _pool + objidx;
    }

private:
#ifdef _REENTRANT
    bee::spinlock _locker;
#endif
    node_type* _pool = nullptr;
    node_type* _free = nullptr;
    size_t _size = 0;
    size_t _cap = 0;
    std::unordered_map<T*, size_t> _obj_map;  // 对象指针到索引的映射
};

//////////////////////////////////////////////////////////////////////////

enum OBJECTPOOL_INIT_MODE
{
    LAZY,  // 延迟初始化
    EAGER, // 预初始化
};

// 无锁对象池：缓存友好、更高效，但初始内存占用大
template<typename T, OBJECTPOOL_INIT_MODE INIT_MODE = LAZY>
class lockfree_objectpool
{
private:
    typedef struct ALIGN_CACHELINE_SIZE memory_block
    {
        T obj;
        std::atomic<memory_block*> next = nullptr;
        bool initialized = false;
    } node_type;

    struct ALIGN_CACHELINE_SIZE head_ptr
    {
        node_type* node = nullptr;
        uint64_t counter = 0;
    };

public:
    lockfree_objectpool() noexcept = default;
    lockfree_objectpool(size_t capacity) { init(capacity); }

    void init(size_t capacity)
    {
        if(_pool) return;
        static_assert(sizeof(node_type) % CACHELINE_SIZE == 0, "memory_block size must be cacheline aligned");
        assert(capacity > 0 && capacity <= OBJECT_POOL_MAX);
        _pool = (node_type*)std::aligned_alloc(CACHELINE_SIZE,  sizeof(node_type) * capacity);
        assert(_pool);

        for(size_t i = 0; i < capacity; ++i)
        {
            node_type* node = &_pool[i];
            if(i != capacity - 1)
                node->next.store(&_pool[i + 1], std::memory_order_relaxed);
            else
                node->next.store(nullptr, std::memory_order_relaxed);

            if constexpr(INIT_MODE == OBJECTPOOL_INIT_MODE::EAGER)
            {
                new (&node->obj) T();
                node->initialized = true;
            }
        }

        head_ptr init_head{_pool, 0};
        _free.store(init_head, std::memory_order_release);
        _cap = capacity;
    }

    ~lockfree_objectpool()
    {
        _free.store(head_ptr{nullptr, 0}, std::memory_order_release);
        for(size_t i = 0; i < _cap; ++i)
        {
            if(_pool[i].initialized)
            {
                _pool[i].obj.~T();
            }
        }
        std::free(_pool);
        _pool = nullptr;
        _cap = 0;
    }

    std::pair<int, T*> alloc()
    {
        head_ptr old_head = _free.load(std::memory_order_acquire);
        while(true)
        {
            if(!old_head.node) return {-1, nullptr};

            node_type* next_node = old_head.node->next.load(std::memory_order_relaxed);
            head_ptr new_head{next_node, old_head.counter + 1};

            if(_free.compare_exchange_weak(old_head, new_head,
                std::memory_order_acq_rel, std::memory_order_acquire))
            {
                node_type* node = old_head.node;
                if constexpr(INIT_MODE == OBJECTPOOL_INIT_MODE::LAZY)
                {
                    if(!node->initialized)
                    {
                        new (&node->obj) T();
                        node->initialized = true;
                    }
                }
                return {node - _pool, &node->obj};
            }
        }
    }

    void free(T* obj)
    {
        node_type* node = (node_type*)obj;
        if(node < _pool || node >= _pool + _cap) return;

        head_ptr old_head = _free.load(std::memory_order_relaxed);
        head_ptr new_head;
        do
        {
            node->next.store(old_head.node, std::memory_order_relaxed);

            new_head.node = node;
            new_head.counter = old_head.counter + 1;
        } while(!_free.compare_exchange_weak(old_head, new_head,
                std::memory_order_acq_rel, std::memory_order_acquire));
    }

    void free(size_t objidx)
    {
        node_type* node = _pool + objidx;
        if(node < _pool || node >= _pool + _cap) return;

        head_ptr old_head = _free.load(std::memory_order_relaxed);
        head_ptr new_head;
        do
        {
            node->next.store(old_head.node, std::memory_order_relaxed);

            new_head.node = node;
            new_head.counter = old_head.counter + 1;
        } while(!_free.compare_exchange_weak(old_head, new_head,
                std::memory_order_acq_rel, std::memory_order_acquire));
    }

    T* find_object(size_t objidx)
    {
        if(objidx >= _cap) return nullptr;
        return &(_pool + objidx)->obj;
    }

private:
    node_type* _pool = nullptr;
    std::atomic<head_ptr> _free;
    size_t _cap = 0;
};

} // namespace bee