#include "http.h"
#include "format.h"
#include <cstring>

namespace bee
{

std::string http_method_to_string(HTTP_METHOD method)
{
#define X(code, name, string) \
    case HTTP_METHOD_##name: return #string;
    switch(method)
    {
        HTTP_METHOD_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_METHOD string_to_http_method(const std::string& method)
{
#define X(code, name, string) \
    if(method.compare(#string) == 0) return HTTP_METHOD_##name;
    HTTP_METHOD_MAP
#undef X
    return HTTP_METHOD::HTTP_METHOD_UNKNOWN;
}

std::string http_status_to_string(HTTP_STATUS status)
{
#define X(code, name, msg) \
    case HTTP_STATUS_##name: return #msg;
    switch(status)
    {
        HTTP_STATUS_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_STATUS string_to_http_status(const std::string& status)
{
#define X(code, name, msg) \
    if(status.compare(#msg) == 0) return HTTP_STATUS_##name;
    HTTP_STATUS_MAP
#undef X
    return HTTP_STATUS::HTTP_STATUS_UNKNOWN;
}

std::string http_result_to_string(HTTP_RESULT result)
{
#define X(code, name, msg) \
    case HTTP_RESULT_##name: return #msg;
    switch(result)
    {
        HTTP_RESULT_MAP
        default: return "Unknown error";
    }
#undef X
}

std::string http_version_to_string(HTTP_VERSION version)
{
#define X(code, name, string) \
    case HTTP_VERSION_##name: return #string;
    switch(version)
    {
        HTTP_VERSION_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_VERSION string_to_http_version(const std::string& version)
{
#define X(code, name, string) \
    if(version.compare(#string) == 0) return HTTP_VERSION_##name;
    HTTP_VERSION_MAP
#undef X
    return HTTP_VERSION::HTTP_VERSION_UNKNOWN;
}

} // namespace bee