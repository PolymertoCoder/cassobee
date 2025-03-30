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

class httpresponse;

class httpprotocol : public protocol
{
public:
    httpprotocol() = default;
    httpprotocol(PROTOCOLID type) : protocol(type) {}
    virtual ~httpprotocol() = default;

    static bool getline(octetsstream& os, std::string& str, char declm = '\n');

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    virtual void encode(octetsstream& os) const override;
    static httpprotocol* decode(octetsstream& os, session* ses);

    void set_body(const std::string& body);
    void set_header(const std::string& key, const std::string& value);

    const std::string& get_body() const { return _body; }
    const std::string& get_header(const std::string& key) const;
    const auto& headers() const { return _headers; }
    auto header_count() const { return _headers.size(); }
    bool has_header(const std::string& key) const { return _headers.contains(key); }

protected:
    friend class httpsession;
    std::string _body;
    std::unordered_map<std::string, std::string> _headers;
};

class httprequest : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPREQUEST;
    httprequest() = default;
    httprequest(PROTOCOLID type) : httpprotocol(type) {}
    virtual ~httprequest() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual size_t maxsize() const override;
    virtual protocol* dup() const override { return new httprequest(*this); }
    virtual void run() override;

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    void set_method(const std::string& method);
    void set_url(const std::string& url);

    const std::string& get_method() const { return _method; }
    const std::string& get_url() const { return _url; }

    void set_callback(std::function<void(httpresponse*)> callback);

private:
    std::string _method;
    std::string _url;
    std::function<void(httpresponse*)> _callback;
};

class httpresponse : public httpprotocol
{
public:
    static constexpr PROTOCOLID TYPE = PROTOCOL_TYPE_HTTPRESPONCE;
    httpresponse() = default;
    httpresponse(PROTOCOLID type) : httpprotocol(type) {}
    virtual ~httpresponse() = default;

    virtual PROTOCOLID get_type() const override { return TYPE; }
    virtual size_t maxsize() const override;
    virtual protocol* dup() const override { return new httpresponse(*this); }
    virtual void run() override;

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    void set_status(int code);
    int get_status() const { return _status; }

private:
    int _status = HTTP_STATUS_OK;
};

static httprequest __register_httprequest(httprequest::TYPE);
static httprequest __register_httpresponse(httpresponse::TYPE);

} // namespace bee
