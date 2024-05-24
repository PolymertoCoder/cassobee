
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