#include "httpprotocol.h"
#include <sstream>

namespace bee
{

httpprotocol::httpprotocol() : protocol(1) {}

void httpprotocol::run()
{
    if (_callback)
    {
        _callback(this);
    }
}

void httpprotocol::set_request(const std::string& method, const std::string& url, const std::string& body)
{
    _method = method;
    _url = url;
    _body = body;
}

void httpprotocol::set_response(int status_code, const std::string& body)
{
    _status_code = status_code;
    _body = body;
}

auto httpprotocol::get_header(const std::string& key) -> const std::string&
{
    static std::string dummy;
    auto iter = _headers.find(key);
    return iter != _headers.end() ? iter->second : dummy;
}

void httpprotocol::set_header(const std::string& key, const std::string& value)
{
    _headers[key] = value;
}

octetsstream& httpprotocol::pack(octetsstream& os) const
{
    std::ostringstream ss;
    if (_method.empty()) // Response
    {
        ss << "HTTP/1.1 " << _status_code << " OK\r\n";
        ss << "Content-Length: " << _body.size() << "\r\n\r\n";
        ss << _body;
    }
    else // Request
    {
        ss << _method << " " << _url << " HTTP/1.1\r\n";
        ss << "Content-Length: " << _body.size() << "\r\n\r\n";
        ss << _body;
    }
    os << ss.str();
}

octetsstream& httpprotocol::unpack(octetsstream& os)
{
    std::string data(os.data().data(), os.size());
    std::istringstream ss(data);
    std::string line;
    std::getline(ss, line);
    if (line.find("HTTP/") == 0) // Response
    {
        _status_code = std::stoi(line.substr(9, 3));
    }
    else // Request
    {
        size_t method_end = line.find(' ');
        size_t url_end = line.find(' ', method_end + 1);
        _method = line.substr(0, method_end);
        _url = line.substr(method_end + 1, url_end - method_end - 1);
    }
    _body = data.substr(data.find("\r\n\r\n") + 4);
}

} // namespace bee