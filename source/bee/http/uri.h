#pragma once
#include "types.h"
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
    bool is_valid() const;
    bool is_default_port() const;

    // getters
    FORCE_INLINE const std::string& get_scheme() const { return _schema; }
    FORCE_INLINE const std::string& get_user() const { return _user; }
    FORCE_INLINE const std::string& get_password() const { return _password; }
    FORCE_INLINE const std::string& get_host() const { return _host; }
    FORCE_INLINE int32_t get_port() const { return _port; }
    FORCE_INLINE const std::string& get_path() const
    {
        static std::string default_path = "/";
        return _path.empty() ? default_path : _path;
    }
    FORCE_INLINE const std::string& get_query() const { return _query; }
    FORCE_INLINE const std::string& get_fragment() const { return _fragment; }

    // setters
    FORCE_INLINE void set_scheme(const std::string& scheme) { _schema = scheme; }
    FORCE_INLINE void set_user(const std::string& user) { _user = user; }
    FORCE_INLINE void set_password(const std::string& password) { _password = password; }
    FORCE_INLINE void set_host(const std::string& host) { _host = host; }
    FORCE_INLINE void set_port(const int32_t port) { _port = port; }
    FORCE_INLINE void set_path(const std::string& path) { _path = path; }
    FORCE_INLINE void set_query(const std::string& query) { _query = query; }
    FORCE_INLINE void set_fragment(const std::string& fragment) { _fragment = fragment; }

protected:
    void parse_authority(const std::string& authority);
    void parse_userinfo(const std::string& userinfo);
    void parse_hostport(const std::string& hostport);

private:
    std::string _schema;
    std::string _user;
    std::string _password;
    std::string _host;
    int32_t     _port;
    std::string _path;
    std::string _query;
    std::string _fragment;
};

} // namespace bee