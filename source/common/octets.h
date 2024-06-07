#pragma once
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include "types.h"

inline size_t frob_size(size_t n)
{
    if(n == 0) return 0;
    n = n - 1;
    n = n | (n >> 1);
    n = n | (n >> 2);
    n = n | (n >> 4);
    n = n | (n >> 8);
    n = n | (n >> 16);
    n = n + 1;
    return n;
}

class octets
{
public:
    octets(const char* data)
    {
        create(data, sizeof(data), sizeof(data));
    }
    octets(const char* data, size_t len)
    {
        create(data, len, len);
    }
    octets(const std::string& str)
    {
        create(str.data(), str.size(), str.size());
    }
    octets(const std::string& str, size_t len)
    {
        create(str.data(), len, len);
    }
    octets(const std::string_view& view)
    {
        create(view.data(), view.size(), view.size());
    }
    octets(const std::string_view& view, size_t len)
    {
        create(view.data(), len, len);
    }
    octets(const octets& oct)
    {
        create(oct.buf(), oct.size(), oct.capacity());
    }
    octets(const octets& oct, size_t len)
    {
        create(oct.buf(), len, len);
    }
    octets(octets&& oct)
    {
        create(oct.buf(), oct.size(), oct.capacity());
    }
    ~octets()
    {
        destroy();
    }

    octets& operator=(const char* rhs)
    {
        create(rhs, sizeof(rhs), sizeof(rhs));
        return *this;
    }
    octets& operator=(const std::string& rhs)
    {
        return *this;
    }
    octets& operator=(const std::string_view& rhs)
    {
        return *this;
    }
    octets& operator=(const octets& rhs)
    {
        create(rhs.buf(), rhs.size(), rhs.capacity());
        return *this;
    }
    octets& operator=(octets&& rhs)
    {
        create(rhs.buf(), rhs.size(), rhs.capacity());
        return *this;
    }
    operator std::string()
    {
        return std::string(_buf, _len);
    }
    operator std::string_view()
    {
        return std::string_view(_buf, _len);
    }
    void append(const char* data, size_t len)
    {
        reserve(frob_size(_len + len));
        strncat(_buf, data, len);
    }
    void reserve(size_t cap)
    {
        if(_cap < cap) return;
        create(_buf, _len, cap);
    }
    
    FORCE_INLINE char* buf() const { return _buf; }
    FORCE_INLINE size_t size() const { return _len; }
    FORCE_INLINE size_t capacity() const { return _cap; }
    FORCE_INLINE size_t free_space() const { return _cap - _len; }
    FORCE_INLINE octets dup() const { return octets(*this); }

private:
    void create(const char* data, size_t len, size_t cap)
    {
        cap = std::max(len, cap);
        char* tmp = new char[cap];
        memcpy(tmp, data, len);
        _len = len;
        _cap = cap;
        delete[] _buf;
        _buf = tmp;
    }
    void destroy()
    {
        delete[] _buf;
        _buf = nullptr;
        _len = 0;
        _cap = 0;
    }

private:
    char*  _buf = nullptr;
    size_t _len = 0;
    size_t _cap = 0;
};