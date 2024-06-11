#pragma once
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include "types.h"

inline size_t frob_size(size_t n)
{
    --n;
    n = n | (n >> 1);
    n = n | (n >> 2);
    n = n | (n >> 4);
    n = n | (n >> 8);
    n = n | (n >> 16);
    ++n;
    return n < 16 ? 16 : n;
}

class octets
{
public:
    octets() : _buf(new char[16]), _len(0), _cap(16)
    {
    }
    octets(const char* data)
    {
        create(data, sizeof(data), sizeof(data));
    }
    octets(const char* begin, const char* end)
    {
        size_t len = end - begin;
        create(begin, len, len);
    }
    octets(const char* data, size_t len)
    {
        create(data, len, len);
    }
    octets(const std::string& str)
    {
        size_t len = str.size();
        create(str.data(), len, len);
    }
    octets(const std::string_view& view)
    {
        size_t len = view.size();
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
    operator char*() const
    {
        return _buf;
    }
    operator const char*() const
    {
        return _buf;
    }
    operator std::string() const
    {
        return std::string(_buf, _len);
    }
    operator std::string_view() const
    {
        return std::string_view(_buf, _len);
    }
    char* operator[](size_t pos) const
    {
        if(pos >= _len) return nullptr;
        return _buf + pos;
    }

    void swap(octets& rhs)
    {
        std::swap(_buf, rhs._buf);
        std::swap(_len, rhs._len);
        std::swap(_cap, rhs._cap);
    }
    void insert(size_t pos, const char* data, size_t len)
    {
        pos = std::min(_len, pos);
        reserve(frob_size(_len + len));
        memmove(_buf + pos + len, _buf + pos, len);
        memmove(_buf + pos, data, len);
        _len += len;
    }
    void append(const char* data, size_t len)
    {
        if(len == 0) return;
        reserve(frob_size(_len + len));
        memmove(_buf + _len, data, len);
        _len += len;
    }
    void erase(size_t pos, size_t len)
    {
        pos = std::min(_len, pos);
        len = std::min(_len - pos, len);
        memmove(_buf, _buf + len, _len - len);
        _len -= len;
    }
    void reserve(size_t cap)
    {
        if(_cap < cap) return;
        create(_buf, _len, cap);
    }

    FORCE_INLINE char* begin() const { return _buf; }
    FORCE_INLINE char* end() const { return _buf + _len; }
    FORCE_INLINE char* buf() const { return _buf; }
    FORCE_INLINE size_t size() const { return _len; }
    FORCE_INLINE size_t capacity() const { return _cap; }
    FORCE_INLINE size_t free_space() const { return _cap - _len; }
    FORCE_INLINE octets dup() const { return octets(*this); }
    FORCE_INLINE void clear() { _len = 0; }

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