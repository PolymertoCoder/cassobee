#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>

namespace bee
{

using TIMETYPE = int64_t;
using SID = uint64_t;
using PROTOCOLID = uint32_t;
using SIG_HANDLER = void(*)(int);

#define PREDICT_TRUE(x)  __builtin_expect(!!(x), 1)
#define PREDICT_FALSE(x) __builtin_expect(!!(x), 0)

#define FORCE_INLINE __attribute__((always_inline))
#define ATTR_WEAK __attribute__((weak))
#define FORMAT_PRINT_CHECK(n, m) __attribute__((format(printf, n, m)))

#ifndef __cpp_lib_hardware_interference_size
namespace std
{
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    inline constexpr size_t hardware_constructive_interference_size = 64;
    inline constexpr size_t hardware_destructive_interference_size  = 64;
}
#endif // __cpp_lib_hardware_interference_size

static_assert(std::hardware_constructive_interference_size >= alignof(std::max_align_t));

constexpr std::size_t CACHELINE_SIZE  = std::hardware_destructive_interference_size;
constexpr std::size_t WORD_SIZE       = sizeof(std::size_t);
constexpr std::size_t CACHELINE_WORDS = std::hardware_destructive_interference_size / WORD_SIZE;

#define ALIGN_CACHELINE_SIZE alignas(std::hardware_constructive_interference_size)

#ifndef _OPEN_VERSION
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

#ifndef _OPEN_VERSION
#define CHECK_BUG(expr, dosth) \
do { \
    if (!(expr)) { \
        assert(false && #expr); \
    } \
} while(0)
#else
#define CHECK_BUG(expr, dosth) \
do { \
    if (!(expr)) { dosth; } \
} while(0)
#endif

#define UNUSE(x) ((void)(x))

} // namespace bee