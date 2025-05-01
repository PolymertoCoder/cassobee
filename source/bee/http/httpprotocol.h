#pragma once
#include "protocol.h"
#include "http.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace bee
{

constexpr PROTOCOLID PROTOCOL_TYPE_HTTPREQUEST  = 200001;
constexpr PROTOCOLID PROTOCOL_TYPE_HTTPRESPONCE = 200002;

class httprequest;
class httpresponse;

class httpprotocol : public protocol
{
public:
    using MAP_TYPE = std::unordered_map<std::string, std::string>;

    httpprotocol() = default;
    virtual ~httpprotocol() = default;

    static bool getline(octetsstream& os, std::string& str, char declm = '\n');

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    virtual void encode(octetsstream& os) const override;
    static httpprotocol* decode(octetsstream& os, session* ses); 

    static httprequest*  get_request();
    static httpresponse* get_response();

    // httpprotocol common method
    void set_websocket(bool is_websocket) { _is_websocket = is_websocket; }
    bool is_websocket() const { return _is_websocket; }

    void set_keepalive(bool is_keepalive) { _is_keepalive = is_keepalive; }
    bool is_keepalive() const { return _is_keepalive; }

    void set_version(HTTP_VERSION version) { _version = version; }
    HTTP_VERSION get_version() const { return _version; }

    void set_body(const std::string& body);
    const std::string& get_body() const { return _body; }

    void set_header(const std::string& key, const std::string& value);
    void del_header(const std::string& key) { _headers.erase(key); }
    const std::string& get_header(const std::string& key) const;
    const MAP_TYPE& get_headers() const { return _headers; }
    size_t get_header_count() const { return _headers.size(); }
    bool has_header(const std::string& key) const { return _headers.contains(key); }

    void set_cookie(const std::string& key, const std::string& value);
    void del_cookie(const std::string& key) { _cookies.erase(key); }

protected:
    friend class httpsession;
    bool _is_websocket = false;
    bool _is_keepalive = false;
    HTTP_VERSION _version;
    std::string  _body;
    MAP_TYPE _headers;
    MAP_TYPE _cookies;
};

class httprequest : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPREQUEST;
    using callback = std::function<void(httpresponse*)>;

    httprequest() = default;
    virtual ~httprequest() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual size_t maxsize() const override;
    virtual httprequest* dup() const override { return new httprequest(*this); }
    virtual void run() override;

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    // httprequest method
    void set_method(HTTP_METHOD method) { _method = method; }
    HTTP_METHOD get_method() const { return _method; }

    void set_url(const std::string& url) { _url = url; }
    const std::string& get_url() const { return _url; }

    void set_query(const std::string& query) { _query = query; }
    const std::string& get_query() const { return _query; }

    void set_fragment(const std::string& fragment) { _fragment = fragment; }
    const std::string& get_fragment() const { return _fragment; }

    void set_param(const std::string& key, const std::string& value) { _params[key] = value; }
    void del_param(const std::string& key) { _params.erase(key); }

private:
    HTTP_METHOD _method;
    std::string _url;       // 请求url
    std::string _query;     // 请求参数
    std::string _fragment;  // 请求fragment
    MAP_TYPE    _params;    // 请求参数map
};

class httpresponse : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPRESPONCE;
    httpresponse() = default;
    virtual ~httpresponse() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual size_t maxsize() const override;
    virtual httpresponse* dup() const override { return new httpresponse(*this); }
    virtual void run() override;

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    // httpresponse method
    void set_status(HTTP_STATUS code) { _status = code; }
    HTTP_STATUS get_status() const { return _status; }

private:
    HTTP_STATUS _status = HTTP_STATUS_OK;
    std::string _reason;
};

} // namespace bee
