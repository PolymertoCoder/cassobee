#include "httpprotocol.h"
#include "config.h"
#include "format.h"
#include "glog.h"
#include "http.h"
#include "httpsession.h"
#include "httpsession_manager.h"
#include "log.h"
#include "marshal.h"
#include "servlet.h"
#include "systemtime.h"
#include "util.h"
#include <cctype>
#include <cstdlib>
#include <string>

namespace bee
{

// httpprotocol implementation

bool httpprotocol::getline(octetsstream& os, std::string& str, char declm)
{
    os >> octetsstream::BEGIN;
    octets& buf = os.data();
    size_t  pos = os.get_pos(), len = 0;
    while(pos < buf.size() && buf[pos] != declm)
    {
        ++pos; ++len;
    }

    if(buf[pos] == declm)
    {
        os.advance(len);
        str = std::string(buf[pos - len], len);
        os >> octetsstream::COMMIT;
        return true;
    }
    else // pos >= buf.size()
    {
        os >> octetsstream::ROLLBACK;
        return false;
    }
}

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
    std::string k, v;
    std::transform(key.begin(), key.end(), k.begin(), ::tolower);
    std::transform(value.begin(), value.end(), v.begin(), ::tolower);
    _headers[k] = v;
}

const std::string& httpprotocol::get_header(const std::string& key) const
{
    static const std::string empty;
    auto iter = _headers.find(key);
    return iter != _headers.end() ? iter->second : empty;
}

octetsstream& httpprotocol::unpack(octetsstream& os)
{
    std::string line;
    auto _getline = [&]()
    {
        if(_parse_state == HTTP_PARSE_STATE_BODY || _parse_state == HTTP_PARSE_STATE_CHUNKED_DATA)
            return true;
        return getline(os, line);
    };

    try
    {
        while(_getline())
        {
            switch(_parse_state)
            {
                case HTTP_PARSE_STATE_HEADER:
                {
                    if(line != "\r")
                    {
                        size_t colon = line.find(':');
                        if(colon != std::string::npos)
                        {
                            std::string key = line.substr(0, colon);
                            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                            std::string value = line.substr(colon + 2, line.size() - colon - 3); // Remove ": " and "\r"
                            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                            _headers[key] = value;
                        }
                    }
                    else
                    {
                        on_parse_header_finished();
                        if(_is_chunked)
                        {
                            _parse_state = HTTP_PARSE_STATE_CHUNKED_SIZE;
                        }
                        else
                        {
                            _parse_state = HTTP_PARSE_STATE_BODY;
                        }
                    }
                } break;
                case HTTP_PARSE_STATE_BODY:
                {
                    if(auto content_length = get_header("content-length"); content_length.size())
                    {
                        os >> octetsstream::BEGIN;
                        int len = std::stoul(content_length);
                        if(!os.data_ready(len))
                        {
                            os >> octetsstream::ROLLBACK;
                            break;
                        }
                        size_t pos = os.get_pos();
                        _body = std::string(os.data()[pos], len);
                        os.advance(len + 2); // skip \r\n
                        os >> octetsstream::COMMIT;
                        _parse_state = HTTP_PARSE_STATE_COMPLETE;
                    }
                } break;
                case HTTP_PARSE_STATE_CHUNKED_SIZE:
                {
                    _chunk_size = std::stoul(line);
                    if(_chunk_size == 0) // chunked body end
                    {
                        _parse_state = HTTP_PARSE_STATE_COMPLETE;
                    }
                    else
                    {
                        _parse_state = HTTP_PARSE_STATE_CHUNKED_DATA;
                    }
                } break;
                case HTTP_PARSE_STATE_CHUNKED_DATA:
                {
                    os >> octetsstream::BEGIN;
                    if(!os.data_ready(_chunk_size))
                    {
                        os >> octetsstream::ROLLBACK;
                        break;
                    }
                    size_t pos = os.get_pos();
                    _body.append(os.data()[pos], _chunk_size);
                    os.advance(_chunk_size + 2); // skip \r\n
                    os >> octetsstream::COMMIT;
                    _chunk_size = 0;
                    _parse_state = HTTP_PARSE_STATE_CHUNKED_SIZE;
                } break;
                default: { break; }
            }
        }
        if(_parse_state == HTTP_PARSE_STATE_COMPLETE)
        {
            local_log("HTTP protocol unpacked successfully.");
        }
    }
    catch(const std::exception& e)
    {
        local_log("Error unpacking HTTP protocol: %s", e.what());
        throw;
    }

    return os;
}

void httpprotocol::encode(octetsstream& os) const
{
    try
    {
        pack(os);
    }
    catch(...)
    {
        local_log("httpprotocol decode failed, id=%d.", _type);
    }
}

httpprotocol* httpprotocol::decode(octetsstream& os, httpsession* httpses)
{
    if(!os.data_ready(1)) return nullptr;
    try
    {
        httpprotocol* temp = httpses->get_unfinished_protocol();
        if(!temp) // new protocol
        {
            os >> octetsstream::BEGIN;
            std::string start_line;
            getline(os, start_line);
            if(start_line.starts_with("HTTP/"))
            {
                temp = get_response();
            }
            else
            {
                temp = get_request();
            }
            os >> octetsstream::ROLLBACK;
        }

        temp->unpack(os);

        if(temp->_parse_state == HTTP_PARSE_STATE_COMPLETE) // 解析完成了
        {
            temp->init_session(httpses);
            httpses->set_unfinished_protocol(nullptr);
            return temp;
        }
        else
        {
            httpses->set_unfinished_protocol(temp);
            return nullptr;
        }
    }
    catch(octetsstream::exception& e)
    {
        httpses->close();
        // local_log("httpprotocol decode throw octetesstream exception %s, id=%d size=%zu.", e.what(), id, size);
    }
    catch(...)
    {
        httpses->close();
        // local_log("httpprotocol decode failed, id=%d size=%zu.", id, size);
    }
    os >> octetsstream::ROLLBACK;
    return nullptr;
}

httprequest* httpprotocol::get_request()
{
    auto req = get_protocol(httprequest::TYPE);
    return (httprequest*)(req->dup());
}

httpresponse* httpprotocol::get_response()
{
    auto rsp = get_protocol(httpresponse::TYPE);
    return (httpresponse*)(rsp->dup());
}

void httpprotocol::on_parse_header_finished()
{
    if(auto connection = get_header("connection"); connection.size())
    {
        if(strcasecmp(connection.data(), "close") == 0)
        {
            _is_keepalive = false;
            local_log("httpprotocol connection close.");
        }
        else if(strcasecmp(connection.data(), "keep-alive") == 0)
        {
            _is_keepalive = true;
            local_log("httpprotocol connection keep-alive.");
        }
        else if(strcasecmp(connection.data(), "upgrade") == 0)
        {
            _is_websocket = true;
            local_log("httpprotocol connection upgrade.");
        }
    }

    if(auto transfer_encoding = get_header("transfer-encoding"); transfer_encoding.size())
    {
        if(strcasecmp(transfer_encoding.data(), "chunked") == 0)
        {
            _is_chunked = true;
            _is_keepalive = true; // 分块传输需要维持长连接
            local_log("httpprotocol transfer-encoding chunked.");
        }
    }
}

// httprequest implementation
size_t httprequest::maxsize() const
{
    return config::get_instance()->get<size_t>("http", "max_request_size", 1024 * 1024);
}

void httprequest::run()
{
    auto* httpserver = dynamic_cast<httpserver_manager*>(_manager);
    if(!httpserver) return;
    if(auto* dispatcher = httpserver->get_dispatcher())
    {
        httpresponse* rsp = get_response();
        rsp->set_version(_version);
        rsp->set_header("server", httpserver->identity());
        dispatcher->handle(this, rsp);
    }
}

ostringstream& httprequest::dump(ostringstream& out) const
{
    out << http_method_to_string(_method) << " "
        << _path
        << (_query.empty() ? "" : "?")
        << _query
        << (_fragment.empty() ? "" : "#")
        << _fragment
        << " HTTP/"
        << http_version_to_string(_version)
        << "\r\n";

    if(!_is_websocket)
    {
        out << "connection: " << (_is_keepalive? "keep-alive" : "close") << "\r\n";
    }
    for(const auto& [key, value] : _headers)
    {
        if(!_is_websocket && strcasecmp(key.data(), "connection") == 0) continue;
        out << key << ": " << value << "\r\n";
    }
    if(!_body.empty())
    {
        out << "content-length: " << _body.size() << "\r\n\r\n";
    }
    else
    {
        out << "\r\n";
    }
    return out;
}

octetsstream& httprequest::pack(octetsstream& os) const
{
    thread_local bee::ostringstream oss;
    oss.clear();
    dump(oss);
    os.data().append(oss.c_str());
    return os;
}

octetsstream& httprequest::unpack(octetsstream& os)
{
    // 解析请求行
    if(_parse_state == HTTP_PARSE_STATE_NONE)
    {
        _parse_state = HTTP_PARSE_STATE_FIRST_LINE;
    }

    if(_parse_state == HTTP_PARSE_STATE_FIRST_LINE)
    {
        std::string line;
        if(!getline(os, line)) return os;
        if(line.empty()) throw exception("Empty request line");

        // method
        size_t method_end = line.find(' ');
        _method = string_to_http_method(line.substr(0, method_end));
        if(_method == HTTP_METHOD_UNKNOWN) throw exception("Unknown HTTP method");

        // path
        size_t path_end = line.find(' ', method_end + 1);
        _path = line.substr(method_end + 1, path_end - method_end - 1);
        if(_path.empty()) throw exception("Empty URL");

        // version
        _version = string_to_http_version(line.substr(path_end + 1));
        if(_version == HTTP_VERSION_UNKNOWN) throw exception("Unknown HTTP version");

        _parse_state = HTTP_PARSE_STATE_HEADER;
    }

    httpprotocol::unpack(os);
    return os;
}

void httprequest::init_param()
{
    init_query_param();
    init_body_param();
    init_cookies();
}

const std::string& httprequest::get_param(const std::string& key)
{
    init_query_param();
    init_body_param();
    static const std::string empty;
    auto iter = _params.find(key);
    return iter != _params.end() ? iter->second : empty;
}

const std::string& httprequest::get_cookie(const std::string& key)
{
    init_cookies();
    static const std::string empty;
    auto iter = _cookies.find(key);
    return iter != _cookies.end() ? iter->second : empty;
}

void httprequest::handle_response(int result, httpresponse* rsp)
{
    if(_callback)
    {
        _callback(result, this, rsp);
    }
}

void httprequest::parse_param(const std::string& str, MAP_TYPE& params, const char* flag, trim_func_type trim_func)
{
    for(const std::string& keyval : bee::split(str, flag))
    {
        size_t pos = keyval.find('=');
        if(pos == std::string::npos) return;
        std::string key   = keyval.substr(0, pos);
        std::string value = keyval.substr(pos + 1);
        if(trim_func) key = trim_func(key);
        value = util::url_decode(value);
        params[key] = value;
    };
}

void httprequest::init_query_param()
{
    if(_parse_param_flag.test(PARSE_QUERY_PARAM)) return;
    parse_param(_query, _params, "&");
    _parse_param_flag.set(PARSE_QUERY_PARAM);
}

void httprequest::init_body_param()
{
    if(_parse_param_flag.test(PARSE_BODY_PARAM)) return;
    if(auto content_type = get_header("content-type"); content_type.size())
    {
        if(strcasestr(content_type.data(), "application/x-www-form-urlencoded"))
        {
            parse_param(_body, _params, "&");
        }
    }
    _parse_param_flag.set(PARSE_BODY_PARAM);
}

void httprequest::init_cookies()
{
    if(_parse_param_flag.test(PARSE_COOKIES)) return;
    auto cookie = get_header("cookie");
    if(cookie.empty()) return;
    parse_param(cookie, _cookies, ";", [](const std::string_view& str) { return bee::trim(str, "\t\r\n"); });
    _parse_param_flag.set(PARSE_COOKIES);
}

// httpresponse implementation

size_t httpresponse::maxsize() const
{
    return config::get_instance()->get<size_t>("http", "max_response_size", 1024 * 1024);
}

void httpresponse::run()
{
    auto* httpclient = dynamic_cast<httpclient_manager*>(_manager);
    if(!httpclient) return;
    httpclient->handle_response(HTTP_RESULT_OK, this);
}

ostringstream& httpresponse::dump(ostringstream& out) const
{
    out << "HTTP/"
        << http_version_to_string(_version)
        << " "
        << _status
        << " "
        << http_status_to_string(_status)
        << "\r\n";
    
    if(!_is_websocket)
    {
        out << "connection: " << (_is_keepalive? "keep-alive" : "close") << "\r\n";
    }
    for(const auto& [key, value] : _headers)
    {
        if(!_is_websocket && strcasecmp(key.data(), "connection") == 0) continue;
        out << key << ": " << value << "\r\n";
    }
    for(const auto& cookie : _cookies)
    {
        out << "set-cookie: " << cookie << "\r\n";
    }
    if(!_body.empty())
    {
        out << "content-length: " << _body.size() << "\r\n\r\n"
            << _body;
    }
    else
    {
        out << "\r\n";
    }
    return out;
}

octetsstream& httpresponse::pack(octetsstream& os) const
{
    thread_local ostringstream oss;
    oss.clear();
    dump(oss);
    os.data().append(oss.c_str());
    return os;
}

octetsstream& httpresponse::unpack(octetsstream& os)
{
    // 解析响应行
    if(_parse_state == HTTP_PARSE_STATE_NONE)
    {
        _parse_state = HTTP_PARSE_STATE_FIRST_LINE;
    }

    if(_parse_state == HTTP_PARSE_STATE_FIRST_LINE)
    {
        // 解析状态行
        std::string line;
        if(!getline(os, line)) return os;
        if(line.empty()) throw exception("Empty request line");

        // version
        size_t version_end = line.find(' ');
        _version = string_to_http_version(line.substr(0, version_end));
        if(_version == HTTP_VERSION_UNKNOWN) throw exception("Unknown HTTP version");

        // status
        size_t status_end = line.find(' ', version_end + 1);
        std::string status_code(line.data() + version_end + 1, status_end - version_end - 1);
        _status = (HTTP_STATUS)(std::atoi(status_code.data()));
        if(_status == HTTP_STATUS_UNKNOWN) throw exception("Unknown HTTP status");

        _parse_state = HTTP_PARSE_STATE_HEADER;
    }

    httpprotocol::unpack(os);
    return os;
}

void httpresponse::set_redirect(const std::string& url)
{
    _status = HTTP_STATUS_FOUND;
    set_header("location", url);
}

void httpresponse::set_cookie(const std::string& key, const std::string& value, TIMETYPE expiretime, const std::string& path, const std::string& domain, bool secure, bool httponly)
{
    thread_local bee::ostringstream oss;
    oss.clear();
    oss << key << "=" << value;
    if(expiretime != 0)
    {
        oss << ";expires=" << systemtime::format_time(expiretime, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if(!domain.empty())
    {
        oss << ";domain=" << domain;
    }
    if(!path.empty())
    {
        oss << ";path=" << path;
    }
    if(secure)
    {
        oss << ";secure";
    }
    if(httponly)
    {
        oss << ";httponly";
    }
    _cookies.emplace_back(oss.str());
}

} // namespace bee