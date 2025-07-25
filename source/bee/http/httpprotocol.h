#pragma once
#include "protocol.h"
#include "http.h"
#include "types.h"
#include <bitset>
#include <string>
#include <unordered_map>
#include <functional>

namespace bee
{

constexpr PROTOCOLID PROTOCOL_TYPE_HTTPREQUEST  = 200001;
constexpr PROTOCOLID PROTOCOL_TYPE_HTTPRESPONCE = 200002;

class httpsession;
class httprequest;
class httpresponse;
class http_callback;
class http_servlet_task;

class httpprotocol : public protocol
{
public:
    using MAP_TYPE = std::unordered_map<std::string, std::string>;

    httpprotocol() = default;
    httpprotocol(PROTOCOLID type) : protocol(type) {}
    virtual ~httpprotocol() = default;

    static bool getline(octetsstream& os, std::string& str, char declm = '\n');

    virtual void run() override {}
    virtual octetsstream& unpack(octetsstream& os) override;
    virtual void encode(octetsstream& os) const override;
    static httpprotocol* decode(octetsstream& os, httpsession* httpses);

    static httprequest*  get_request();
    static httpresponse* get_response();

    // httpprotocol common method
    FORCE_INLINE void set_websocket(bool is_websocket) { _is_websocket = is_websocket; }
    FORCE_INLINE bool is_websocket() const { return _is_websocket; }

    FORCE_INLINE void set_keepalive(bool is_keepalive) { _is_keepalive = is_keepalive; }
    FORCE_INLINE bool is_keepalive() const { return _is_keepalive; }

    FORCE_INLINE void set_version(HTTP_VERSION version) { _version = version; }
    FORCE_INLINE HTTP_VERSION get_version() const { return _version; }

    void set_body(const std::string& body);
    FORCE_INLINE const std::string& get_body() const { return _body; }

    void set_header(const std::string& key, const std::string& value);
    FORCE_INLINE void del_header(const std::string& key) { _headers.erase(key); }
    const std::string& get_header(const std::string& key) const;
    FORCE_INLINE const MAP_TYPE& get_headers() const { return _headers; }
    FORCE_INLINE size_t get_header_count() const { return _headers.size(); }
    FORCE_INLINE bool has_header(const std::string& key) const { return _headers.contains(key); }

protected:
    void on_parse_header_finished();

protected:
    friend class httpsession;
    HTTP_PARSE_STATE _parse_state = HTTP_PARSE_STATE_NONE;
    bool _is_chunked = false;
    size_t _chunk_size = 0;
    bool _is_websocket = false;
    bool _is_keepalive = false;
    HTTP_VERSION _version;
    std::string  _body;
    MAP_TYPE _headers;
};

class httprequest : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPREQUEST;
    using callback = http_callback;

    httprequest() = default;
    httprequest(PROTOCOLID type) : httpprotocol(type) {}
    virtual ~httprequest() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual const char* get_name() const override { return "httprequest"; }
    virtual size_t maxsize() const override;
    virtual httprequest* dup() const override { return new httprequest(*this); }
    virtual ostringstream& dump(ostringstream& out) const override;
    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    // httprequest method
    void init_param();

    FORCE_INLINE void set_method(HTTP_METHOD method) { _method = method; }
    FORCE_INLINE HTTP_METHOD get_method() const { return _method; }

    FORCE_INLINE void set_path(const std::string& path) { _path = path; }
    FORCE_INLINE const std::string& get_path() const { return _path; }

    FORCE_INLINE void set_query(const std::string& query) { _query = query; }
    FORCE_INLINE const std::string& get_query() const { return _query; }

    FORCE_INLINE void set_fragment(const std::string& fragment) { _fragment = fragment; }
    FORCE_INLINE const std::string& get_fragment() const { return _fragment; }

    FORCE_INLINE void set_param(const std::string& key, const std::string& value) { _params[key] = value; }
    const std::string& get_param(const std::string& key);
    FORCE_INLINE void del_param(const std::string& key) { _params.erase(key); }

    FORCE_INLINE void set_cookie(const std::string& key, const std::string& value) { _cookies[key] = value; }
    const std::string& get_cookie(const std::string& key);
    FORCE_INLINE bool has_cookie(const std::string& key) const { return _cookies.contains(key); }
    FORCE_INLINE void del_cookie(const std::string& key) { _cookies.erase(key); }

protected:
    enum
    {
        PARSE_QUERY_PARAM,
        PARSE_BODY_PARAM,
        PARSE_COOKIES,
        PARSE_FLAG_SIZE,
    };
    std::bitset<PARSE_FLAG_SIZE> _parse_param_flag;
    using trim_func_type = std::function<std::string(const std::string_view&)>;
    void parse_param(const std::string& str, MAP_TYPE& params, const char* flag, trim_func_type trim_func = {});
    void init_query_param();
    void init_body_param();
    void init_cookies();

private:
    friend class http_callback_task;
    friend class httpclient;
    HTTP_METHOD _method;    // HTTP方法
    std::string _path;      // 请求路径
    std::string _query;     // 请求参数
    std::string _fragment;  // 请求fragment
    MAP_TYPE    _params;    // 请求参数map
    MAP_TYPE    _cookies;   // 请求cookie
};

class httpresponse : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPRESPONCE;
    using task = http_servlet_task;

    httpresponse() = default;
    httpresponse(PROTOCOLID type) : httpprotocol(type) {}
    virtual ~httpresponse() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual const char* get_name() const override { return "httpresponse"; }
    virtual size_t maxsize() const override;
    virtual httpresponse* dup() const override { return new httpresponse(*this); }
    virtual ostringstream& dump(ostringstream& out) const override;
    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    // httpresponse method
    FORCE_INLINE void set_status(HTTP_STATUS code) { _status = code; }
    FORCE_INLINE HTTP_STATUS get_status() const { return _status; }

    void set_redirect(const std::string& url);
    void set_cookie(const std::string& key, const std::string& value, TIMETYPE expiretime = 0, const std::string& path = "/", const std::string& domain = "", bool secure = false, bool httponly = false);

private:
    HTTP_STATUS _status = HTTP_STATUS_OK;
    std::vector<std::string> _cookies;
    std::string _reason;
};

} // namespace bee
