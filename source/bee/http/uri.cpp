#include "uri.h"

namespace bee
{

uri::uri(const std::string& uri_str)
{
    parse(uri_str);
}

void uri::parse(const std::string& uri_str)
{
    // Parse the URI string and populate the member variables
    // This is a placeholder implementation
    size_t pos = uri_str.find("://");
    if (pos != std::string::npos) {
        _schema = uri_str.substr(0, pos);
        size_t authority_start = pos + 3;
        size_t authority_end = uri_str.find('/', authority_start);
        if (authority_end != std::string::npos) {
            _authority = uri_str.substr(authority_start, authority_end - authority_start);
            size_t user_pos = _authority.find('@');
            if (user_pos != std::string::npos) {
                _user = _authority.substr(0, user_pos);
                _authority.erase(0, user_pos + 1);
            }
            size_t port_pos = _authority.find(':');
            if (port_pos != std::string::npos) {
                _host = _authority.substr(0, port_pos);
                _port = _authority.substr(port_pos + 1);
            } else {
                _host = _authority;
            }
        }
        size_t path_start = authority_end;
        size_t query_start = uri_str.find('?', path_start);
        if (query_start != std::string::npos) {
            _path = uri_str.substr(path_start, query_start - path_start);
            size_t fragment_start = uri_str.find('#', query_start);
            if (fragment_start != std::string::npos) {
                _query = uri_str.substr(query_start + 1, fragment_start - query_start - 1);
                _fragment = uri_str.substr(fragment_start + 1);
            } else {
                _query = uri_str.substr(query_start + 1);
            }
        } else {
            size_t fragment_start = uri_str.find('#', path_start);
            if (fragment_start != std::string::npos) {
                _path = uri_str.substr(path_start, fragment_start - path_start);
                _fragment = uri_str.substr(fragment_start + 1);
            } else {
                _path = uri_str.substr(path_start);
            }
        }
    }
    else {
        // Handle the case where the URI does not have a scheme
        _path = uri_str;
    }
}

void uri::clear()
{
    _schema.clear();
    _user.clear();
    _password.clear();
    _host.clear();
    _port.clear();
    _path.clear();
    _query.clear();
    _fragment.clear();
    _authority.clear();
}

std::string uri::to_string() const
{
    std::string result;
    if (!_schema.empty()) {
        result += _schema + "://";
    }
    if (!_user.empty()) {
        result += _user + "@";
    }
    if (!_host.empty()) {
        result += _host;
    }
    if (!_port.empty()) {
        result += ":" + _port;
    }
    if (!_path.empty()) {
        result += _path;
    }
    if (!_query.empty()) {
        result += "?" + _query;
    }
    if (!_fragment.empty()) {
        result += "#" + _fragment;
    }
    return result;
}

} // namespace bee