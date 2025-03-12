#pragma once
#include "types.h"
#include <cstring>
#include <fstream>

namespace bee
{

template<size_t N>
class fixed_buffer
{
public:
    fixed_buffer()
    {
        _cursor = 0;
        memset(_data, 0, sizeof(_data));
    }
    /**
     * @description: 写接口，不会覆盖写
     * @param {char*} src：数据来源
     * @param {size_t} length：期望写入的数据的长度(bytes)
     * @return {size_t}：实际写入的长度(bytes)
     */    
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
    /**
     * @description: 读接口
     * @param {char*} dest：读取的目的地
     * @param {size_t} length：期望读取的数据长度(bytes)
     * @return {size_t}：实际读取的长度(bytes)
     */    
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
    /**
     * @description: 读取到文件
     * @param {fstream&} stream：文件流
     * @param {size_t} length：期望读取的数据长度(bytes)
     * @return {size_t}：实际读取的长度(bytes)
     */    
    size_t read(std::fstream& stream, size_t length)
    {
        size_t avail_length = _cursor;
        size_t read_length  = (length > avail_length) ? avail_length : length;
        if(read_length > 0)
        {
            stream.write(_data, read_length);
            _cursor -= read_length;
        }
        return read_length;
    }

    FORCE_INLINE void reset() { _cursor = 0; }
    FORCE_INLINE size_t free_space() const { return N - _cursor; }
    FORCE_INLINE size_t size() const { return _cursor; }
    FORCE_INLINE bool empty() const { return _cursor == 0; }
    FORCE_INLINE constexpr size_t capacity() const { return N; }

private:
    size_t _cursor = 0;
    char   _data[N];
};

}