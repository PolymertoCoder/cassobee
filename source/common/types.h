#pragma once
#include <stdint.h>

using TIMETYPE = int64_t;
using SID = uint64_t;
using PROTOCOLID = uint32_t;
using SIG_HANDLER = void(*)(int);

#define PREDICT_TRUE(x)  __builtin_expect(!!(x), 1)
#define PREDICT_FALSE(x) __builtin_expect(!!(x), 0)

#define FORCE_INLINE __attribute__((always_inline))
#define ATTR_WEAK __attribute__((weak))
#define FORMAT_PRINT_CHECK(n, m) __attribute__((format(printf, n, m)))

#ifndef _OPEN_VERSION
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

#ifndef _OPEN_VERSION
#define CHECK_BUG(expr, dosth) \
if(!expr) \
{ \
    assert(false); \
}
#else
#define CHECK_BUG(expr) \
if(!expr) \
{ \
    dosth; \
}
#endif

#define UNUSE(x) ((void)x)
