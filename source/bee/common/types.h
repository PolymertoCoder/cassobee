#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>

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
if(!(expr)) \
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

#define UNUSE(x) ((void)(x))

enum HTTP_METHOD
{
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    TRACE,
    CONNECT,
};

enum HTTP_STATUS
{
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORT_MEDIA_TYPE = 415,
    RANGET_NOT_SATISFIABLE = 416,
    EXPECTION_FAILED = 417,
    UPGRADE_REQUIRED = 426,
    PERCONDITION_FAILED = 428,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    NETWORK_AUTHENTICATION_REQUIRED = 511,
};

enum HTTP_VERSION
{
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0, // Future support
};

constexpr PROTOCOLID HTTPREQUEST_TYPE  = 200001;
constexpr PROTOCOLID HTTPRESPONCE_TYPE = 200002;
