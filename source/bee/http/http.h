#pragma once
#include <string>

namespace bee
{

class ostringstream;

#define HTTP_METHOD_MAP \
    X(0, UNKNOWN, "Unknown") \
    X(1, GET, "GET") \
    X(2, POST, "POST") \
    X(3, PUT, "PUT") \
    X(4, DELETE, "DELETE") \
    X(5, HEAD, "HEAD") \
    X(6, OPTIONS, "OPTIONS") \
    X(7, PATCH, "PATCH") \
    X(8, TRACE, "TRACE") \
    X(9, CONNECT, "CONNECT")

#define HTTP_STATUS_MAP \
    X(0, UNKNOWN, "Unknown") \
    X(100, CONTINUE, "Continue") \
    X(101, SWITCHING_PROTOCOLS, "Switching Protocols") \
    X(102, PROCESSING, "Processing") \
    X(200, OK, "OK") \
    X(201, CREATED, "Created") \
    X(202, ACCEPTED, "Accepted") \
    X(203, NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information") \
    X(204, NO_CONTENT, "No Content") \
    X(205, RESET_CONTENT, "Reset Content") \
    X(206, PARTIAL_CONTENT, "Partial Content") \
    X(207, MULTI_STATUS, "Multi Status") \
    X(208, ALREADY_REPORTED, "Already Reported") \
    X(226, IM_USED, "Im Used") \
    X(300, MULTIPLE_CHOICES, "Multiple Choices") \
    X(301, MOVED_PERMANENTLY, "Moved Permanently") \
    X(302, FOUND, "Found") \
    X(303, SEE_OTHER, "See Other") \
    X(304, NOT_MODIFIED, "Not Modified") \
    X(307, TEMPORARY_REDIRECT, "Temporary Redirect") \
    X(308, PERMANENT_REDIRECT, "Permanent Redirect") \
    X(400, BAD_REQUEST, "Bad Request") \
    X(401, UNAUTHORIZED, "Unauthorized") \
    X(403, FORBIDDEN, "Forbidden") \
    X(404, NOT_FOUND, "Not Found") \
    X(405, METHOD_NOT_ALLOWED, "Method Not Allowed") \
    X(408, REQUEST_TIMEOUT, "Request Timeout") \
    X(409, CONFLICT, "Conflict") \
    X(410, GONE, "Gone") \
    X(411, LENGTH_REQUIRED, "Length Required") \
    X(413, PAYLOAD_TOO_LARGE, "Payload Too Large") \
    X(414, URI_TOO_LONG, "URI Too Long") \
    X(415, UNSUPPORT_MEDIA_TYPE, "Unsupported Media Type") \
    X(416, RANGET_NOT_SATISFIABLE, "Range Not Satisfiable") \
    X(417, EXPECTION_FAILED, "Expectation Failed") \
    X(426, UPGRADE_REQUIRED, "Upgrade Required") \
    X(428, PERCONDITION_FAILED, "Precondition Required") \
    X(429, TOO_MANY_REQUESTS, "Too Many Requests") \
    X(431, REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large") \
    X(500, INTERNAL_SERVER_ERROR, "Internal Server Error") \
    X(501, NOT_IMPLEMENTED, "Not Implemented") \
    X(502, BAD_GATEWAY, "Bad Gateway") \
    X(503, SERVICE_UNAVAILABLE, "Service Unavailable") \
    X(504, GATEWAY_TIMEOUT, "Gateway Timeout") \
    X(505, HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported") \
    X(511, NETWORK_AUTHENTICATION_REQUIRED, "Network Authentication Required") \

// 内部错误码定义（负数）
#define HTTP_RESULT_MAP \
    X( 0, OK, "OK") \
    X(-1, TIMEOUT, "Timeout") \
    X(-2, INVALID_URL, "Invalid URL") \
    X(-3, INVALID_HOST, "Invalid host") \
    X(-4, CONNECT_FAIL, "Connection failed") \
    X(-5, SEND_CLOSE_BY_PEER, "Send closed by peer") \
    X(-6, WAIT_CLOSE_BY_PEER, "Wait closed by peer") \
    X(-7, SEND_SOCKET_ERROR, "Send socket error") \
    X(-8, SSL_NOT_ENABLED, "SSL not enabled") \
    X(-9, CREATE_SOCKET_ERROR, "Create socket error") \
    X(-10, POOL_GET_CONNECTION_ERROR, "Get connection from pool error") \
    X(-11, POOL_INVALID_CONNECTION, "Invalid connection from pool")

#define HTTP_HEADER_MAP \
    X(0, UNKNOWN, "Unknown") \
    X(1, CONTENT_LENGTH, "Content-Length") \
    X(2, CONTENT_TYPE, "Content-Type") \
    X(3, HOST, "Host") \
    X(4, USER_AGENT, "User-Agent") \
    X(5, ACCEPT, "Accept") \
    X(6, CONNECTION, "Connection") \
    X(7, TRANSFER_ENCODING, "Transfer-Encoding") \
    X(8, UPGRADE_INSECURE_REQUESTS, "Upgrade-Insecure-Requests") \
    X(9, ORIGIN, "Origin")

#define HTTP_VERSION_MAP \
    X(0, UNKNOWN, "Unknown") \
    X(1, 1_0, "HTTP/1.0") \
    X(2, 1_1, "HTTP/1.1") \
    X(3, 2_0, "HTTP/2.0")

/*
UNKNOWN = 0, // 未知方法
GET,         // 请求指定的页面信息，并返回实体主体
POST,        // 向指定资源提交数据进行处理请求（例如提交表单或者上传文件）。数据被包含在请求体中。POST请求可能会导致新的资源的建立和/或已有资源的修改。
PUT,         // 从客户端向服务器传送的数据取代指定的文档的内容。
DELETE,      // 请求服务器删除指定的页面。
HEAD,        // 类似于get请求，只不过返回的响应中没有具体的内容，用于获取报头
OPTIONS,     // 允许客户端查看服务器的性能。
PATCH,       // 
TRACE,       // 回显服务器收到的请求，主要用于测试或诊断。
CONNECT,     // HTTP/1.1协议中预留给能够将连接改为管道方式的代理服务器。
*/
enum HTTP_METHOD
{
#define X(code, name, desc) HTTP_METHOD_##name = code, // NOLINT
    HTTP_METHOD_MAP
#undef X
};

enum HTTP_STATUS
{
#define X(code, name, desc) HTTP_STATUS_##name = code, // NOLINT
    HTTP_STATUS_MAP
#undef X
};

enum HTTP_RESULT
{
#define X(code, name, desc) HTTP_RESULT_##name = code, // NOLINT
    HTTP_RESULT_MAP
#undef X
};

enum HTTP_VERSION
{
#define X(code, name, desc) HTTP_VERSION_##name = code, // NOLINT
    HTTP_VERSION_MAP
#undef X
};

enum HTTP_PARSE_STATE
{
    HTTP_PARSE_STATE_NONE = 0,
    HTTP_PARSE_STATE_FIRST_LINE,
    HTTP_PARSE_STATE_HEADER,
    HTTP_PARSE_STATE_BODY,
    HTTP_PARSE_STATE_CHUNKED_SIZE,
    HTTP_PARSE_STATE_CHUNKED_DATA,
    HTTP_PARSE_STATE_CHUNKED_END,
    HTTP_PARSE_STATE_COMPLETE,
    HTTP_PARSE_STATE_ERROR,
};

std::string http_method_to_string(HTTP_METHOD method);
HTTP_METHOD string_to_http_method(const std::string& method);

std::string http_status_to_string(HTTP_STATUS status);
HTTP_STATUS string_to_http_status(const std::string& status);

std::string http_result_to_string(HTTP_RESULT result);
HTTP_RESULT string_to_http_result(const std::string& result);

std::string http_version_to_string(HTTP_VERSION version);
HTTP_VERSION string_to_http_version(const std::string& version);

} // namespace bee