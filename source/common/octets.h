#pragma once
#include <cstddef>
#include "types.h"

namespace std
{
    class string;
    class string_view;
}

class octets
{
public:
    octets(const char* data);
    octets(const char* data, size_t size);
    octets(const std::string& str);
    octets(const std::string_view& str);

    octets(const octets& oct);
    octets(const octets& oct, size_t size);
    octets(octets&& oct);

    octets& operator=(const char* rhs);
    octets& operator=(const octets& rhs);
    octets& operator=(octets&& rhs);
    octets& operator=(const std::string& rhs);
    octets& operator=(const std::string_view& rhs);

    ~octets();

    void append(const char* data, size_t size);
    void reserve(size_t size);

    octets dup();

    FORCE_INLINE size_t size() const { return _len; }
    FORCE_INLINE size_t capacity() const { return _cap; }
    FORCE_INLINE size_t free_space() const { return _cap - _len; }

public:
    void create(const char* data, size_t size)
    {
        if(_data)
        {
            delete[] _data;
        }
        _data = new char[size];
        _len = 0;
        _cap  = size;
    }
    void destroy()
    {
        if(_data)
        {
            delete[] _data;
            _data = nullptr;
        }
        _len = 0;
        _cap  = 0;
    }

private:
    char* _data  = nullptr;
    size_t _len = 0;
    size_t _cap  = 0;
};