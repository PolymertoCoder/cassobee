#pragma once
#include "concept.h"
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <netinet/in.h>

inline uint64_t htonll(uint64_t host)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return ((uint64_t)htonl(host & 0xFFFFFFFF) << 32) | htonl(host >> 32);
#else
    return host;
#endif
}

inline uint64_t ntohll(uint64_t net)
{
#if BYTE_ORDER == LITTLE_ENDIAN
        return ((uint64_t)ntohl(net & 0xFFFFFFFF) << 32) | ntohl(net >> 32);
#else
        return net;
#endif
}

template<typename T>
T hostToNetwork(T value);

template<bee::pod T>
T hostToNetwork(T value)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    size_t size = sizeof(T);
    char* data = (char*)&value;
    for(size_t i = 0; i < size / 2; i++)
    {
        char temp = data[i];
        data[i] = data[size - i - 1];
        data[size - i - 1] = temp;
    }
#endif
    return value;
}

template<>
inline uint64_t hostToNetwork<uint64_t>(uint64_t host64)
{
    return htonll(host64);
}

template<>
inline int64_t hostToNetwork<int64_t>(int64_t host64)
{
    return htonll(host64);
}

template<>
inline uint32_t hostToNetwork<uint32_t>(uint32_t host32)
{
    return htonl(host32);
}

template<>
inline int32_t hostToNetwork<int32_t>(int32_t host32)
{
    return htonl(host32);
}

template<>
inline uint16_t hostToNetwork<uint16_t>(uint16_t host16)
{
    return htons(host16);
}

template<>
inline int16_t hostToNetwork<int16_t>(int16_t host16)
{
    return htons(host16);
}

template<>
inline uint8_t hostToNetwork<uint8_t>(uint8_t host8)
{
    return host8;
}

template<>
inline int8_t hostToNetwork<int8_t>(int8_t host8)
{
    return host8;
}

template<>
inline bool hostToNetwork<bool>(bool host8)
{
    return host8;
}

template<>
inline float hostToNetwork<float>(float host)
{
    uint32_t temp;
    std::memcpy(&temp, &host, sizeof(float));
    temp = hostToNetwork(temp);
    float net;
    std::memcpy(&net, &temp, sizeof(float));
    return net;
}

template<>
inline double hostToNetwork<double>(double host)
{
    uint64_t temp;
    std::memcpy(&temp, &host, sizeof(double));
    temp = hostToNetwork(temp);
    double net;
    std::memcpy(&net, &temp, sizeof(double));
    return net;
}

template<typename T>
T networkToHost(T value);

template<bee::pod T>
T networkToHost(T value)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    size_t size = sizeof(T);
    char* data = (char*)&value;
    for(size_t i = 0; i < size / 2; i++)
    {
        char temp = data[i];
        data[i] = data[size - i - 1];
        data[size - i - 1] = temp;
    }
#endif
    return value;
}

template<>
inline uint64_t networkToHost<uint64_t>(uint64_t host64)
{
    return ntohll(host64);
}

template<>
inline int64_t networkToHost<int64_t>(int64_t host64)
{
    return ntohll(host64);
}

template<>
inline uint32_t networkToHost<uint32_t>(uint32_t host32)
{
    return ntohl(host32);
}

template<>
inline int32_t networkToHost<int32_t>(int32_t host32)
{
    return ntohl(host32);
}

template<>
inline uint16_t networkToHost<uint16_t>(uint16_t host16)
{
    return ntohs(host16);
}

template<>
inline int16_t networkToHost<int16_t>(int16_t host16)
{
    return ntohs(host16);
}

template<>
inline uint8_t networkToHost<uint8_t>(uint8_t host8)
{
    return host8;
}

template<>
inline int8_t networkToHost<int8_t>(int8_t host8)
{
    return host8;
}

template<>
inline bool networkToHost<bool>(bool host8)
{
    return host8;
}

template<>
inline float networkToHost<float>(float net)
{
    uint32_t temp;
    std::memcpy(&temp, &net, sizeof(float));
    temp = networkToHost(temp);
    float host;
    std::memcpy(&host, &temp, sizeof(float));
    return host;
}

template<>
inline double networkToHost<double>(double net)
{
    uint64_t temp;
    std::memcpy(&temp, &net, sizeof(double));
    temp = networkToHost(temp);
    double host;
    std::memcpy(&host, &temp, sizeof(double));
    return host;
}