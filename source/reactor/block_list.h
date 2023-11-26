#include <iostream>
#include <memory>
#include <string.h>
#include <cmath>

#define BLOCK_CAPACITY_DEFAULT 16
#define ITEMS_COUNT_PER_BLOCK 128

template<typename T>
class block_list
{
public:
    struct block
    {
        T* _items;
        block* _next;
        T& operator[](size_t idx)
        {
            if(idx > ITEMS_COUNT_PER_BLOCK) throw "items access out of range!!!";
            return _items[idx];
        }
    };

    struct iterator
    {
        size_t _idx;
        block* _cur;

        iterator(size_t offset = 0, block* pblock = nullptr) : _idx(offset), _cur(pblock) {}
        ~iterator() {}
        T& operator*() const { return _cur[_idx]; }
        T* operator->() const { return _cur + _idx; }
        iterator& operator++()
        {
            if(++_idx >= ITEMS_COUNT_PER_BLOCK)
            {
                _cur = _cur->_next;
                if(_cur == nullptr){ return block_list::_dummy; }
                _idx = 0;
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator ==(const iterator& other) const { return _cur ==  other->_cur && _idx == other->_idx; }
        bool operator !=(const iterator& other) const { return _cur !=  other->_cur || _idx != other->_idx; }
    };

public:
    block_list(size_t size = 0)
    {
        if(size == 0) {
            allocate(BLOCK_CAPACITY_DEFAULT);
        } else {
            allocate(ceil(size / ITEMS_COUNT_PER_BLOCK));
        }
    }
    ~block_list()
    {
        if(_block_map)
        {
            block* cur = _block_map;
            for(size_t i = 0; i < _block_size; ++i)
            {
                if(cur->_items){ delete[] cur->_items; }
                block* next = cur->_next;
                delete cur;
                cur = next;
            }
        }
    }

    void insert(size_t pos, const T& value)
    {
        resize(pos + 1);
        size_t which = pos / ITEMS_COUNT_PER_BLOCK;
        size_t idx = pos % ITEMS_COUNT_PER_BLOCK;
        block* pblock = _block_map;
        while(which--){ pblock = pblock->_next; }
        (*pblock)[idx] = value;
    }

    T& operator[](size_t pos)
    {
        resize(pos);
        size_t which = pos / ITEMS_COUNT_PER_BLOCK;
        size_t idx = pos % ITEMS_COUNT_PER_BLOCK;
        block* pblock = _block_map;
        while(which--){ pblock = pblock->_next; }
        return (*pblock)[idx];
    }

    size_t max_size()
    {
        return _max_size;
    }

    iterator begin()
    {
        if(_max_size == 0 || _block_map == nullptr) return block_list::_dummy;
        return iterator(_block_map, 0);
    }
    iterator end(){ return block_list::_dummy; }

private:
    void resize(size_t size)
    {
        if(size <= _max_size) return;
        size_t which = size / ITEMS_COUNT_PER_BLOCK;
        allocate(which + 1 - _block_size);
    }

    void allocate(size_t block_size)
    {
        block* prev = _tail;
        for(size_t i = 0; i < block_size; ++i)
        {
            block* pblock = new block;
            if(pblock == nullptr){ throw "create block error!!!"; }
            memset(pblock, 0, sizeof(pblock));
            if(_block_map == nullptr){ _block_map = pblock;  }

            pblock->_items = new T[ITEMS_COUNT_PER_BLOCK];
            if(pblock->_items == nullptr){ throw "create items error!!!"; }
            memset(pblock->_items, 0, sizeof(pblock->_items));

            if(i == block_size - 1)
            {
                _tail = pblock;
                _tail->_next = nullptr;
            }
            if(prev){ prev->_next = pblock; }
            prev = pblock;
        }
        _block_size += block_size;
        _max_size += block_size * ITEMS_COUNT_PER_BLOCK;
    }
    void deallocate()
    {

    }

private:
    block* _block_map = nullptr;
    block* _tail = nullptr;
    size_t _block_size = 0;
    size_t _max_size = 0;

    static iterator _dummy;
};
