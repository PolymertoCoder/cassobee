#pragma once
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include "types.h"

inline constexpr size_t frob_size(size_t n)
{
    if(n > (size_t(1) << 63))
    {
        throw std::overflow_error("frob_size: input too large");
    }
    if(n == 0) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
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
        size_t len = std::strlen(data);
        create(data, len, len);
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
        this->swap(oct);
    }
    ~octets()
    {
        destroy();
    }

    octets& operator=(const char* rhs)
    {
        size_t len = std::strlen(rhs);
        create(rhs, len, len);
        return *this;
    }
    octets& operator=(const std::string& rhs)
    {
        create(rhs.data(), rhs.size(), rhs.size());
        return *this;
    }
    octets& operator=(const std::string_view& rhs)
    {
        create(rhs.data(), rhs.size(), rhs.size());
        return *this;
    }
    octets& operator=(const octets& rhs)
    {
        if(&rhs != this)
        {
            create(rhs.buf(), rhs.size(), rhs.capacity());
        }
        return *this;
    }
    octets& operator=(octets&& rhs)
    {
        if(&rhs != this)
        {
            this->swap(rhs);
        }
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
    char& operator[](size_t pos) const
    {
        if(pos >= _len)
        {
            throw std::out_of_range("octets::operator[]: index out of range");
        }
        return _buf[pos];
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
        reserve(_len + len);
        memmove(_buf + pos + len, _buf + pos, _len - pos);
        memcpy(_buf + pos, data, len);
        _len += len;
        //printf("insert\n");
    }
    void append(const char* data, size_t len)
    {
        if(len == 0) return;
        reserve(_len + len);
        memcpy(_buf + _len, data, len);
        _len += len;
        //printf("append\n");
    }
    void replace(size_t pos, const char* data, size_t len)
    {
        if(len == 0) return;
        ASSERT(_len - pos >= len);
        len = std::min(_len - pos, len); // 不改变长度
        reserve(pos + len);
        memcpy(_buf + pos, data, len);
        _len = std::max(_len, pos + len);
        //printf("replace\n");
    }
    void erase(size_t pos, size_t len)
    {
        pos = std::min(_len, pos);
        len = std::min(_len - pos, len);
        memmove(_buf + pos, _buf + pos + len, _len - pos - len);
        _len -= len;
        //printf("erase\n");
    }
    void reserve(size_t cap)
    {
        if(_cap >= cap) return;
        create(_buf, _len, frob_size(cap));
        //printf("reserve\n");
    }

    FORCE_INLINE char* begin() const { return _buf; }
    FORCE_INLINE char* end() const { return _buf + _len; }
    FORCE_INLINE char* data() const { return _buf; }
    FORCE_INLINE char* buf() const { return _buf; }
    FORCE_INLINE char* peek(size_t pos) const { return _buf + pos; }
    FORCE_INLINE size_t size() const { return _len; }
    FORCE_INLINE size_t capacity() const { return _cap; }
    FORCE_INLINE size_t free_space() const { return _cap - _len; }
    FORCE_INLINE octets dup() const { return octets(*this); }
    FORCE_INLINE void fast_resize(size_t len) { _len += len; }
    FORCE_INLINE void clear() { _len = 0; }

private:
    void create(const char* data, size_t len, size_t cap)
    {
        cap = std::max(len, cap);
        //printf("create: newcap=%zu oldcap=%zu\n", cap, _cap);
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