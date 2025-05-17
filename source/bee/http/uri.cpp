#include "uri.h"
#include "format.h"

namespace bee
{

uri::uri(const std::string& uri_str)
{
    parse(uri_str);
}

void uri::clear()
{
    _schema.clear();
    _user.clear();
    _password.clear();
    _host.clear();
    _port = 0;
    _path.clear();
    _query.clear();
    _fragment.clear();
}

// 格式：schema:[//authority]path[?query][#fragment]
// 其中 authority = [userinfo@]host[:port]
void uri::parse(const std::string& uri_str)
{
    clear();
    size_t pos = uri_str.find(':');
    
    // 解析 Scheme
    if(pos != std::string::npos)
    {
        _schema = uri_str.substr(0, pos);
        std::string rest = uri_str.substr(pos + 1);
        
        // 处理分层 URI
        if(rest.size() >= 2 && rest[0] == '/' && rest[1] == '/')
        {
            size_t authority_start = 2;
            size_t authority_end = rest.find_first_of("/?#", authority_start);
            
            // 解析 Authority
            std::string authority = rest.substr(authority_start, authority_end - authority_start);
            parse_authority(authority);

            // 解析剩余部分
            size_t path_start = (authority_end != std::string::npos) ? authority_end : rest.size();
            std::string remaining = rest.substr(path_start);
            
            // 分离 Fragment
            size_t frag_pos = remaining.find('#');
            if(frag_pos != std::string::npos)
            {
                _fragment = remaining.substr(frag_pos + 1);
                remaining = remaining.substr(0, frag_pos);
            }
            
            // 分离 Query
            size_t query_pos = remaining.find('?');
            if(query_pos != std::string::npos)
            {
                _query = remaining.substr(query_pos + 1);
                remaining = remaining.substr(0, query_pos);
            }
            
            _path = remaining;
        }
        else // 非分层 URI
        {
            _path = rest;
        }
    }
    else // 无 Scheme 的情况
    {
        // 分离 Fragment
        size_t frag_pos = uri_str.find('#');
        if(frag_pos != std::string::npos)
        {
            _fragment = uri_str.substr(frag_pos + 1);
            _path = uri_str.substr(0, frag_pos);
        }
        else
        {
            _path = uri_str;
        }
        
        // 分离 Query
        size_t query_pos = _path.find('?');
        if(query_pos != std::string::npos)
        {
            _query = _path.substr(query_pos + 1);
            _path = _path.substr(0, query_pos);
        }
    }
}

std::string uri::to_string() const
{
    thread_local bee::ostringstream result;
    result.clear();

    if(!_schema.empty())
    {
        result << _schema << ":";
        if(!_host.empty() || _port > 0 || !_user.empty())
        {
            result << "//";
        }
    }
    
    if(!_user.empty())
    {
        result << _user;
        if(!_password.empty())
        {
            result << ":" << _password;
        }
        result << "@";
    }
    
    if(!_host.empty())
    {
        if(_host.find(':') != std::string::npos)
        {
            result << "[" << _host << "]"; // IPv6 地址
        }
        else
        {
            result << _host;
        }
    }

    result << (is_default_port() ? "" : ":" + std::to_string(_port));
    
    if(!_path.empty())
    {
        // 确保路径以斜杠开头（仅限分层URI）
        if(!result.empty() && _path[0] != '/' && !_host.empty())
        {
            result << "/";
        }
        result << _path;
    }
    
    if(!_query.empty())
    {
        result << "?" << _query;
    }
    
    if(!_fragment.empty())
    {
        result << "#" << _fragment;
    }
    
    return result.str();
}

bool uri::is_valid() const
{
    return !_schema.empty() || !_host.empty() || !_path.empty();
}

bool uri::is_default_port() const
{
    if(_port == 0) return true;
    if(_schema == "http" || _schema == "ws")
    {
        return _port == 80;
    }
    else if(_schema == "https" || _schema == "wss")
    {
        return _port == 443;
    }
    return false;
}

// 解析Authority
void uri::parse_authority(const std::string& authority)
{
    size_t user_pos = authority.find('@');
    if(user_pos != std::string::npos)
    {
        parse_userinfo(authority.substr(0, user_pos));
        parse_hostport(authority.substr(user_pos + 1));
    }
    else
    {
        parse_hostport(authority);
    }
}

// 解析用户信息
void uri::parse_userinfo(const std::string& userinfo)
{
    size_t colon_pos = userinfo.find(':');
    if(colon_pos != std::string::npos)
    {
        _user = userinfo.substr(0, colon_pos);
        _password = userinfo.substr(colon_pos + 1);
    }
    else
    {
        _user = userinfo;
    }
}

// 解析主机和端口
void uri::parse_hostport(const std::string& hostport)
{
    if(hostport.empty()) return;
    
    // 处理 IPv6 地址，例如 http://[2001:db8::1]:8080
    if(hostport[0] == '[')
    {
        size_t closing_bracket = hostport.find(']', 1);
        if(closing_bracket != std::string::npos)
        {
            _host = hostport.substr(1, closing_bracket - 1);
            size_t colon_pos = hostport.find(':', closing_bracket);
            if(colon_pos != std::string::npos)
            {
                _port = std::atoi(hostport.substr(colon_pos + 1).data());
            }
        }
    }
    else
    {
        size_t colon_pos = hostport.find(':');
        if(colon_pos != std::string::npos)
        {
            _host = hostport.substr(0, colon_pos);
            _port = std::atoi(hostport.substr(colon_pos + 1).data());
        }
        else
        {
            _host = hostport;
        }
    }
}

} // namespace bee