#pragma once

#define PREDICT_TRUE(x)  __builtin_expect(!!(x), 1)
#define PREDICT_FALSE(x) __builtin_expect(!!(x), 0)

#define expr2boolstr(expr) (expr) ? "true" : "false"

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

#define ONEHOUR 3600   // 60*60
#define ONEDAY  86400  // 3600*24
#define ONEWEEK 604800 // 3600*24*7
