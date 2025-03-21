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
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
};

enum HTTP_STATUS
{
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_SEE_OTHER = 303,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_TEMPORARY_REDIRECT = 307,
    HTTP_STATUS_PERMANENT_REDIRECT = 308,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_REQUEST_TIMEOUT = 408,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_GONE = 410,
    HTTP_STATUS_LENGTH_REQUIRED = 411,
    HTTP_STATUS_PAYLOAD_TOO_LARGE = 413,
    HTTP_STATUS_URI_TOO_LONG = 414,
    HTTP_STATUS_UNSUPPORT_MEDIA_TYPE = 415,
    HTTP_STATUS_RANGET_NOT_SATISFIABLE = 416,
    HTTP_STATUS_EXPECTION_FAILED = 417,
    HTTP_STATUS_UPGRADE_REQUIRED = 426,
    HTTP_STATUS_PERCONDITION_FAILED = 428,
    HTTP_STATUS_TOO_MANY_REQUESTS = 429,
    HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504,
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED = 511,
};

enum HTTP_VERSION
{
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0, // Future support
};

constexpr PROTOCOLID HTTPREQUEST_TYPE  = 200001;
constexpr PROTOCOLID HTTPRESPONCE_TYPE = 200002;


} // namespace bee