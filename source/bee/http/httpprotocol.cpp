#include "httpprotocol.h"
#include "config.h"
#include <print>
#include <sstream>

namespace bee
{

// httpprotocol implementation

void httpprotocol::set_body(const std::string& body)
{
    _body = body;
}

void httpprotocol::set_header(const std::string& key, const std::string& value)
{
    if (key.empty() || value.empty())
    {
        throw std::invalid_argument("Header key or value cannot be empty");
    }
    _headers[key] = value;
}

const std::string& httpprotocol::get_header(const std::string& key) const
{
    static const std::string empty;
    auto iter = _headers.find(key);
    return iter != _headers.end() ? iter->second : empty;
}

octetsstream& httpprotocol::pack(octetsstream& os) const
{
    std::ostringstream ss;
    for (const auto& [key, value] : _headers)
    {
        ss << key << ": " << value << "\r\n";
    }
    ss << "Content-Length: " << _body.size() << "\r\n\r\n";
    ss << _body;
    os << ss.str();
    return os;
}

octetsstream& httpprotocol::unpack(octetsstream& os)
{
    std::string data(os.data().data(), os.size());
    std::istringstream ss(std::move(data));
    std::string line;

    try
    {
        while (std::getline(ss, line) && line != "\r")
        {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 2, line.size() - colon - 3); // Remove ": " and "\r"
                _headers[key] = value;
            }
        }

        if (ss.eof())
        {
            throw std::runtime_error("Malformed HTTP headers");
        }

        _body = std::string(std::istreambuf_iterator<char>(ss), {});
    }
    catch (const std::exception& e)
    {
        std::println("Error unpacking HTTP protocol: %s", e.what());
        throw;
    }

    return os;
}

// httprequest implementation
size_t httprequest::maxsize() const
{
    return config::get_instance()->get<size_t>("http", "max_request_size", 1024 * 1024);
}

void httprequest::run()
{
    if (_callback)
    {
        auto response = new httpresponse();
        _callback(response);
        delete response;
    }
}

void httprequest::set_method(const std::string& method)
{
    _method = method;
}

void httprequest::set_url(const std::string& url)
{
    _url = url;
}

void httprequest::set_callback(std::function<void(httpresponse*)> callback)
{
    _callback = std::move(callback);
}

octetsstream& httprequest::pack(octetsstream& os) const
{
    std::ostringstream ss;
    ss << _method << " " << _url << " HTTP/1.1\r\n";
    httpprotocol::pack(os);
    return os;
}

octetsstream& httprequest::unpack(octetsstream& os)
{
    std::string data(os.data().data(), os.size());
    std::istringstream ss(data);
    std::string line;
    std::getline(ss, line);
    size_t method_end = line.find(' ');
    size_t url_end = line.find(' ', method_end + 1);
    _method = line.substr(0, method_end);
    _url = line.substr(method_end + 1, url_end - method_end - 1);
    httpprotocol::unpack(os);
    return os;
}

// httpresponse implementation

void httpresponse::set_status(int code)
{
    _status = code;
}

size_t httpresponse::maxsize() const
{
    return config::get_instance()->get<size_t>("http", "max_response_size", 1024 * 1024);
}

void httpresponse::run()
{

}

octetsstream& httpresponse::pack(octetsstream& os) const
{
    std::ostringstream ss;
    ss << "HTTP/1.1 " << _status << " OK\r\n";
    httpprotocol::pack(os);
    return os;
}

octetsstream& httpresponse::unpack(octetsstream& os)
{
    std::string data(os.data().data(), os.size());
    std::istringstream ss(data);
    std::string line;
    std::getline(ss, line);
    _status = std::stoi(line.substr(9, 3));
    httpprotocol::unpack(os);
    return os;
}

} // namespace bee