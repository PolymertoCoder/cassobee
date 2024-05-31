#pragma once
#include "lock.h"
#include "types.h"
#include <cstring>

namespace cassobee
{

template<size_t N>
class fixed_buffer
{
public:
    fixed_buffer()
    {
        memset(_data, 0, sizeof(_data));
    }
    size_t write(const char* src, size_t length)
    {
        size_t free_space   = this->free_space();
        size_t write_length = (length > free_space) ? free_space : length;
        if(write_length > 0)
        {
            std::memcpy(_data + _cursor, src, write_length);
            _cursor += write_length;
        }
        return write_length;
    }
    size_t read(char* dest, size_t length)
    {
        size_t avail_length = _cursor;
        size_t read_length  = (length > avail_length) ? avail_length : length;
        if(read_length > 0)
        {
            std::memcpy(dest, _data, read_length);
            _cursor -= read_length;
        }
        return read_length;
    }

    FORCE_INLINE void reset() {_cursor = 0;}
    FORCE_INLINE size_t free_space() const { return N - _cursor; }
    FORCE_INLINE size_t size() const { return _cursor; }
    FORCE_INLINE constexpr size_t capacity() const { return N; }

private:
    size_t _cursor;
    char   _data[N];
};

}