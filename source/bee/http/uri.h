#pragma once
#include <string>

namespace bee
{

class uri
{
public:
    uri() = default;
    uri(const std::string& uri_str);

    void parse(const std::string& uri_str);
    void clear();

    std::string to_string() const;

    // Getters
    const std::string& scheme() const { return _schema; }
    const std::string& user() const { return _user; }
    const std::string& password() const { return _password; }
    const std::string& host() const { return _host; }
    const std::string& port() const { return _port; }
    const std::string& path() const { return _path; }
    const std::string& query() const { return _query; }
    const std::string& fragment() const { return _fragment; }
    const std::string& authority() const { return _authority; }

    // Setters
    void set_scheme(const std::string& scheme) { _schema = scheme; }
    void set_user(const std::string& user) { _user = user; }
    void set_password(const std::string& password) { _password = password; }
    void set_host(const std::string& host) { _host = host; }
    void set_port(const std::string& port) { _port = port; }
    void set_path(const std::string& path) { _path = path; }
    void set_query(const std::string& query) { _query = query; }
    void set_fragment(const std::string& fragment) { _fragment = fragment; }
    void set_authority(const std::string& authority) { _authority = authority; }

private:
    std::string _schema;
    std::string _user;
    std::string _password;
    std::string _host;
    std::string _port;
    std::string _path;
    std::string _query;
    std::string _fragment;
    std::string _authority;
};

} // namespace bee