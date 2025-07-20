#pragma once
#include <cstdint>
#include <vector>

namespace bee
{

// zigzag 编码/解码
inline constexpr uint16_t encode_zigzag16(int16_t n)
{
    return (n << 1) ^ (n >> 15);
}

inline constexpr int16_t decode_zigzag16(uint16_t n)
{
    return (n >> 1) ^ -static_cast<int16_t>(n & 1);
}

inline constexpr uint32_t encode_zigzag32(int32_t n)
{
    return (n << 1) ^ (n >> 31);
}

inline constexpr int32_t decode_zigzag32(uint32_t n)
{
    return (n >> 1) ^ -static_cast<int32_t>(n & 1);
}

inline constexpr uint64_t encode_zigzag64(int64_t n)
{
    return (n << 1) ^ (n >> 63);
}

inline constexpr int64_t decode_zigzag64(uint64_t n)
{
    return (n >> 1) ^ -static_cast<int64_t>(n & 1);
}

// varint 编码/解码
inline void write_varint(uint64_t value, std::vector<char>& buffer)
{
    while(value >= 0x80)
    {
        buffer.push_back(static_cast<char>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    buffer.push_back(static_cast<char>(value));
}

inline const char* read_varint(const char* ptr, const char* end, uint64_t& value)
{
    value = 0;
    int shift = 0;

    while(ptr < end && shift < 64)
    {
        uint8_t byte = static_cast<uint8_t>(*ptr++);
        value |= (static_cast<uint64_t>(byte & 0x7F) << shift);
        if((byte & 0x80) == 0)
            return ptr;
        shift += 7;
    }

    return nullptr;
}

namespace
{

// 编码表：bit length → compact_int 编码所需字节数
constexpr uint8_t compact_int_size_table[65] = {
    // bits: 0 ~ 64
    1,1,1,1,1,1,1,1,   // 0~7 bits → 1 byte
    2,2,2,2,2,2,2,2,   // 8~14 bits → 2 bytes
    3,3,3,3,3,3,3,3,   // 15~21 bits → 3 bytes
    4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,
    9 // 64 bits → 9 bytes
};

// 获取 bit length（最高有效位的索引 + 1）
template<std::integral T>
inline size_t bit_length(T value)
{
    if (value == 0) return 0;
    return sizeof(size_t) * 8 - std::countl_zero(value);
}

}

// 主函数：获取 size_t 值在 compact_int 编码下所需的字节数
template<std::integral T>
inline size_t get_compact_int_size(T value)
{
    size_t bits = bit_length(value);
    return compact_int_size_table[bits];
}

} // namespace bee
