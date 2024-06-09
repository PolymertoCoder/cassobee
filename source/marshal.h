#pragma once

#include "octets.h"
#include "types.h"
#include <exception>
#include <type_traits>

class octetsstream;

class marshal
{
public:
    virtual octetsstream& pack(octetsstream&)   = 0;
    virtual octetsstream& unpack(octetsstream&) = 0;
};

class octetsstream
{
public:
    class exception : public std::exception
    {
    public:
        exception(const char* msg) : _msg(msg) {}
        FORCE_INLINE virtual const char* what() const noexcept override
        {
            return _msg;
        }
    private:
        const char* _msg; 
    };

public:
    octetsstream(const octets& oct) : _data(oct) {}
    octetsstream(const octets& oct, size_t size) : _data(oct, size) {}

    template<typename T>
    octetsstream& operator <<(const T& val)
    {
        return push(val);
    }
    template<typename T>
    octetsstream& operator >>(T& val)
    {
        return pop(val);
    }

    template<typename T>
    requires std::is_standard_layout_v<T>
    octetsstream& push(const T& val)
    {
        _data.append(&val, sizeof(T));
        return *this;
    }
    template<typename T>
    requires std::is_standard_layout_v<T>
    octetsstream& pop(T& val)
    {
        size_t len = sizeof(val);
        if(_pos + len > _data.size())
        {
            throw exception("no enough data!!!");
        }
        memcpy(&val, _data.begin() + _pos, len);
        _pos += len;
        return *this;
    }

    template<typename T>
    requires std::is_standard_layout_v<T>
    octetsstream& push_front(const T& val)
    {
        size_t len = sizeof(T);
        _data.insert(0, &val, len);
        _pos += len; // pos指向原来的位置
        return *this;
    }
    template<typename T>
    requires std::is_standard_layout_v<T>
    octetsstream& pop_front(T& val)
    {
        size_t len = sizeof(val);
        if(_pos + len > _data.size())
        {
            throw exception("no enough data!!!");
        }
        memcpy(&val, _data.begin(), len);
        _data.erase(0, len);
        _pos -= len; // pos指向原来的位置
        return *this;
    }

    void swap(octetsstream& rhs)
    {
        _data.swap(rhs._data);
        std::swap(_pos, rhs._pos);
        std::swap(_transpos, rhs._transpos);
    }
    void reserve(size_t cap)
    {
        _data.reserve(cap);
    }

    FORCE_INLINE octets& data() { return _data; }
    FORCE_INLINE void reset_pos() { _pos = 0; }
    FORCE_INLINE size_t get_pos() const { return _pos; }
    FORCE_INLINE size_t size() const { return _data.size(); }
    FORCE_INLINE size_t capacity() const { return _data.capacity(); }

private:
    octets _data;
    size_t _pos = 0;
    size_t _transpos = 0;
};