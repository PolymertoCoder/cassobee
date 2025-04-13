#include "http.h"
#include "format.h"
#include <cstring>

namespace bee
{

std::string http_method_to_string(HTTP_METHOD method)
{
#define X(code, name, desc) \
    case HTTP_METHOD_##name: return #name;
    switch(method)
    {
        HTTP_METHOD_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_METHOD string_to_http_method(const std::string& method)
{
#define X(code, name, desc) \
    if(strcmp(method.c_str(), #name)) return HTTP_METHOD_##name;
    HTTP_METHOD_MAP
#undef X
    return HTTP_METHOD::HTTP_METHOD_UNKNOWN;
}

std::string http_status_to_string(HTTP_STATUS status)
{
#define X(code, name, desc) \
    case HTTP_STATUS_##name: return #name;
    switch(status)
    {
        HTTP_STATUS_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_STATUS string_to_http_status(const std::string& status)
{
#define X(code, name, desc) \
    if(strcmp(status.c_str(), #name)) return HTTP_STATUS_##name;
    HTTP_STATUS_MAP
#undef X
    return HTTP_STATUS::HTTP_STATUS_UNKNOWN;
}

std::string http_version_to_string(HTTP_VERSION version)
{
#define X(code, name, desc) \
    case HTTP_VERSION_##name: return #name;
    switch(version)
    {
        HTTP_VERSION_MAP
        default: return "Unknown";
    }
#undef X
}

HTTP_VERSION string_to_http_version(const std::string& version)
{
#define X(code, name, desc) \
    if(strcmp(version.c_str(), #name)) return HTTP_VERSION_##name;
    HTTP_VERSION_MAP
#undef X
    return HTTP_VERSION::HTTP_VERSION_UNKNOWN;
}

ostringstream& operator<<(ostringstream& oss, HTTP_METHOD method)
{
    oss << http_method_to_string(method);
    return oss;
}

ostringstream& operator<<(ostringstream& oss, HTTP_STATUS status)
{
    oss << http_status_to_string(status);
    return oss;
}

ostringstream& operator<<(ostringstream& oss, HTTP_VERSION version)
{
    oss << http_version_to_string(version);
    return oss;
}

} // namespace bee