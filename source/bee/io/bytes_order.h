#pragma once
#include <cstdint>
#include <endian.h>
#include <type_traits>

template<typename T>
requires std::is_arithmetic_v<T>
T bytes_order(T value)
{
    return value;
}

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

template<typename T>
requires std::is_arithmetic_v<T>
T hostToNetwork(T value)
{
    return value;
}

template<>
inline uint64_t hostToNetwork<uint64_t>(uint64_t host64)
{
  return htobe64(host64);
}

template<>
inline uint32_t hostToNetwork<uint32_t>(uint32_t host32)
{
  return htobe32(host32);
}

template<>
inline uint16_t hostToNetwork<uint16_t>(uint16_t host16)
{
  return htobe16(host16);
}

template<typename T>
requires std::is_arithmetic_v<T>
T networkToHost(T value)
{
    return value;
}

template<>
inline uint64_t networkToHost<uint64_t>(uint64_t host64)
{
  return htobe64(host64);
}

template<>
inline uint32_t networkToHost<uint32_t>(uint32_t host32)
{
  return htobe32(host32);
}

template<>
inline uint16_t networkToHost<uint16_t>(uint16_t host16)
{
  return htobe16(host16);
}
