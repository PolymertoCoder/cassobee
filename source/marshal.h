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
        virtual const char* what() const noexcept override
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
    requires std::is_standard_layout_v<T>
    void push(const T& val)
    {
        _data.append(&val, sizeof(T));
    }
    template<typename T> void pop(T& val)
    {
        
    }

    template<> void push<bool>(const bool& val)
    {
        
    }

    template<typename T> void push_front();
    template<typename T> void pop_front();

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

    FORCE_INLINE size_t get_pos() const { return _pos; }

private:
    octets _data;
    size_t _pos = 0;
    size_t _transpos;
};