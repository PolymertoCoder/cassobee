#pragma once
#include "types.h"
#include <cstring>
#include <fstream>

namespace cassobee
{
/**
 * @description: 环形缓冲区的实现
 * @return {*}
 */
template<size_t N>
class ring_buffer
{
public:
    ring_buffer()
    {
        _write_cursor = 0;
        _read_cursor  = 0;
        _size = 0;
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
        length = std::min(free_space(), length);
        size_t written = 0;
        while(written < length)
        {
            size_t write_length = std::min(N - _write_cursor, length - written);
            std::memcpy(_data + _write_cursor, src + written, write_length);
            _write_cursor = (_write_cursor + write_length) % N;
            written += write_length;
        }
        _size += written;
        //printf("write finish: written:%zu write_cursor:%zu read_cursor:%zu _size:%zu.\n", written, _write_cursor, _read_cursor, _size);
        return written;
    }
    /**
     * @description: 读接口
     * @param {char*} dest：读取的目的地
     * @param {size_t} length：期望读取的数据长度(bytes)
     * @return {size_t}：实际读取的长度(bytes)
     */    
    size_t read(char* dest, size_t length)
    {
        length = std::min(size(), length);
        size_t read = 0;
        while(read < length)
        {
            size_t read_length  = std::min(N - _read_cursor, length - read);
            std::memcpy(dest + read, _data + _read_cursor, read_length);
            _read_cursor = (_read_cursor + read_length) % N;
            read += read_length;
        }
        _size -= read;
        return read;
    }
    /**
     * @description: 读取到文件
     * @param {fstream&} stream：文件流
     * @param {size_t} length：期望读取的数据长度(bytes)
     * @return {size_t}：实际读取的长度(bytes)
     */    
    size_t read(std::fstream& stream, size_t length)
    {
        length = std::min(size(), length);
        size_t read = 0;
        while(read < length)
        {
            size_t read_length  = std::min(N - _read_cursor, length - read);
            stream.write(_data + _read_cursor, read_length);
            _read_cursor = (_read_cursor + read_length) % N;
            read += read_length;
        }
        _size -= read;
        //printf("read finish: read:%zu write_cursor:%zu read_cursor:%zu _size:%zu.\n", read, _write_cursor, _read_cursor, _size);
        return read;
    }
    void reset()
    {
        _write_cursor = 0;
        _read_cursor  = 0;
        _size = 0;
    }

    FORCE_INLINE size_t free_space() const { return N - _size; }
    FORCE_INLINE size_t size() const { return _size; }
    FORCE_INLINE bool empty() const { return _size == 0; }
    FORCE_INLINE bool full() const { return _size == N; }
    FORCE_INLINE constexpr size_t capacity() const { return N; }

private:
    size_t _write_cursor = 0;
    size_t _read_cursor  = 0;
    size_t _size = 0;
    char   _data[N];
};

}