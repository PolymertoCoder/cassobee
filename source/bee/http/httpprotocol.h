#pragma once
#include "protocol.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace bee
{

class httpprotocol : public protocol
{
public:
    httpprotocol();
    virtual ~httpprotocol() = default;

    virtual PROTOCOLID get_type() const override { return 1; } // Example ID
    virtual size_t maxsize() const override { return 8192; }  // Max HTTP size
    virtual protocol* dup() const override { return new httpprotocol(*this); }
    virtual void run() override;

    void set_request(const std::string& method, const std::string& url, const std::string& body = "");
    void set_response(int status_code, const std::string& body = "");

    const std::string& get_method() const { return _method; }
    const std::string& get_url() const { return _url; }
    const std::string& get_body() const { return _body; }
    int get_status_code() const { return _status_code; }

    auto& headers() const { return _headers; }
    auto header_count() const { return _headers.size();}
    bool has_header(const std::string& key) const { return _headers.contains(key); }
    auto get_header(const std::string& key) -> const std::string&;
    void set_header(const std::string& key, const std::string& value);

    void set_callback(std::function<void(httpprotocol*)> callback) { _callback = std::move(callback); }

protected:
    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

private:
    std::string _method;
    std::string _url;
    std::string _body;
    int _status_code = 200;
    std::unordered_map<std::string, std::string> _headers;
    std::function<void(httpprotocol*)> _callback;
};

} // namespace bee