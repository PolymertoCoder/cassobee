#pragma once
#include <string>

namespace bee
{

class uri
{
public:
    uri() = default;
    uri(const std::string& uri_str);
    void clear();
    void parse(const std::string& uri_str);
    std::string to_string() const;

    // getters
    const std::string& get_scheme() const { return _schema; }
    const std::string& get_user() const { return _user; }
    const std::string& get_password() const { return _password; }
    const std::string& get_host() const { return _host; }
    const std::string& get_port() const { return _port; }
    const std::string& get_path() const { return _path; }
    const std::string& get_query() const { return _query; }
    const std::string& get_fragment() const { return _fragment; }

    // setters
    void set_scheme(const std::string& scheme) { _schema = scheme; }
    void set_user(const std::string& user) { _user = user; }
    void set_password(const std::string& password) { _password = password; }
    void set_host(const std::string& host) { _host = host; }
    void set_port(const std::string& port) { _port = port; }
    void set_path(const std::string& path) { _path = path; }
    void set_query(const std::string& query) { _query = query; }
    void set_fragment(const std::string& fragment) { _fragment = fragment; }

protected:
    void parse_authority(const std::string& authority);
    void parse_userinfo(const std::string& userinfo);
    void parse_hostport(const std::string& hostport);

private:
    std::string _schema;
    std::string _user;
    std::string _password;
    std::string _host;
    std::string _port;
    std::string _path;
    std::string _query;
    std::string _fragment;
};

} // namespace bee