#pragma once
#include <cstdint>
#include <cstring>
#include <endian.h>

template<typename T>
T bytes_order(T value);

template<>
inline int16_t bytes_order<int16_t>(int16_t value)
{
    return htobe16(value);
}

template<>
inline uint16_t bytes_order<uint16_t>(uint16_t value)
{
    return htobe16(value);
}
template<>
inline int32_t bytes_order<int32_t>(int32_t value)
{
    return htobe32(value);
}

template<>
inline uint32_t bytes_order<uint32_t>(uint32_t value)
{
    return htobe32(value);
}

template<>
inline float bytes_order<float>(float value)
{
    return htobe32(value);
}

template<>
inline int64_t bytes_order<int64_t>(int64_t value)
{
    return htobe64(value);
}

template<>
inline uint64_t bytes_order<uint64_t>(uint64_t value)
{
    return htobe64(value);
}

template<>
inline double bytes_order<double>(double value)
{
    return htobe64(value);
}