#include "log.h"
#include "macros.h"
#include "systemtime.h"
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <cstdarg>
#include <functional>
#include <map>
#include <ostream>

namespace cassobee
{

std::string to_string(LOG_LEVEL level)
{
    switch(level)
    {
        case LOG_LEVEL_DEBUG: { return "DEBUG"; } break;
        case LOG_LEVEL_INFO:  { return "INFO";  } break;
        case LOG_LEVEL_WARN:  { return "WARN";  } break;
        case LOG_LEVEL_ERROR: { return "ERROR"; } break;
        case LOG_LEVEL_FATAL: { return "FATAL"; } break;
        default:{ return "UNKNOWN"; } break;
    }
}

void log_event::assign(std::string filename, int line, TIMETYPE time, int threadid, int fiberid, std::string elapse, std::string content)
{
    _filename = std::move(filename);
    _line = line;
    _time = time;
    _threadid = threadid;
    _fiberid = fiberid;
    _elapse = std::move(elapse);
    _content = content;
}

class message_format_item : public log_formatter::format_item
{
public:
    message_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_content();
    }
};

class loglevel_format_item : public log_formatter::format_item
{
public:
    loglevel_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << to_string(level);
    }
};

class elapse_format_item : public log_formatter::format_item
{
public:
    elapse_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_elapse();
    }
};

class threadid_format_item : public log_formatter::format_item
{
public:
    threadid_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_threadid();
    }
};

class fiberid_format_item : public log_formatter::format_item
{
public:
    fiberid_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_fiberid();
    }
};

class datetime_format_item : public log_formatter::format_item
{
public:
    datetime_format_item(const std::string& fmt = "%Y-%m-%d %H:%M:%S") : _fmt(fmt) {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << systemtime::format_time(event->get_time(), _fmt.data());
    }
private:
    std::string _fmt;
};

class filename_format_item : public log_formatter::format_item
{
public:
    filename_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_filename();
    }
};

class line_format_item : public log_formatter::format_item
{
public:
    line_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_line();
    }
};

class newline_format_item : public log_formatter::format_item
{
public:
    newline_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << std::endl;
    }
};

class string_format_item : public log_formatter::format_item
{
public:
    string_format_item(const std::string& str) : _str(str) {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << _str;
    }
private:
    std::string _str;
};

class tab_format_item : public log_formatter::format_item
{
public:
    tab_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << "\t";
    }
private:
    std::string m_string;
};

log_formatter::log_formatter(const std::string pattern)
    : _pattern(pattern)
{
    // str, format, type
    // %xxx %xxx{xxx} %%
    std::vector<std::tuple<std::string, std::string, bool>> vec;
    std::string nstr;
    for(size_t i = 0; i < _pattern.size(); ++i)
    {
        if(_pattern[i] != '%')
        {
            nstr.append(1, _pattern[i]);
            continue;
        }

        if((i + 1) < _pattern.size())
        {
            if(_pattern[i + 1] == '%') // 处理"%%"的情况
            {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        bool parse_fmt = false; // 是否开始解析格式
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < _pattern.size())
        {
            if(parse_fmt == false && (!isalpha(_pattern[n]) && _pattern[n] != '{' && _pattern[n] != '}'))
            { 
                str = _pattern.substr(i + 1, n - i - 1);
                break; 
            }
            if(!parse_fmt)
            {
                if(_pattern[n] == '{')
                {
                    str = _pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    parse_fmt = true; 
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if(parse_fmt == true)
            {
                if(_pattern[n] == '}')
                {
                    fmt = _pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    parse_fmt = false;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == _pattern.size())
            {
                if(str.empty()) // 如果到最后了，直接取完剩余部分
                {
                    str = _pattern.substr(i + 1);
                }
            }
        }

        if(!parse_fmt)
        {
            if(!nstr.empty())
            {
                vec.push_back(std::make_tuple(nstr, "", true));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, false));
            i = n - 1;
        }
        else
        {
            // 格式错误，可能是缺少'}'
            printf("pattern parse error: %s-%s\n", _pattern.data(), _pattern.substr(i).data());
            _error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, true)); 
        }
    }

    if(!nstr.empty())
    {
        vec.push_back(std::make_tuple(nstr, "", true));
    }
    static std::map<std::string, std::function<format_item*(const std::string&)>> format_items_creators =
    {
    #define creator(str, C) \
        { #str, [](const std::string& fmt) { return (format_item*)(new C(fmt)); } }
    
        creator(m, message_format_item),
        creator(p, loglevel_format_item),
        creator(r, elapse_format_item),
        creator(t, threadid_format_item),
        creator(n, newline_format_item),
        creator(d, datetime_format_item),
        creator(f, filename_format_item),
        creator(l, line_format_item),
        creator(T, tab_format_item),
        creator(F, fiberid_format_item)
    #undef creator
    };

    for(auto& [key, fmt, isstr] : vec)
    {
        if(isstr)
        {
            _items.push_back(new string_format_item(key));
        }
        else
        {
            auto iter = format_items_creators.find(key);
            if(iter == format_items_creators.end())
            {
                _items.push_back(new string_format_item("<<error_format %" + key + ">>"));
                _error = true;
            }
            else
            {
                _items.push_back(iter->second(fmt));
            }
        }

        printf("(%s-%s-%s)\n", key.data(), fmt.data(), expr2boolstr(isstr));
    }
    printf("format_items size:%zu", _items.size());
}

std::string log_formatter::format(LOG_LEVEL level, log_event* event)
{
    std::stringstream os;
    for(format_item* item : _items)
    {
        item->format(os, level, event);
    }
    return os.str();
}

file_appender::file_appender(std::string filedir, std::string filename)
    : _filedir(filedir), _filename(filename)
{
    if(_filedir.empty())
    {
        _filedir = ".";
    }
    _filepath = _filedir + '/' + filename;
}

bool file_appender::init()
{
    int ret = mkdir(_filedir.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(ret != 0 && errno != EEXIST)
    {
        printf("mkdir failed, dir:%s err:%s", _filedir.data(), strerror(errno));
        return false;
    }
    _filestream.open(_filepath, std::fstream::out | std::fstream::app);
    return true;
}

uint64_t file_appender::get_days_suffix()
{
    // 按自然日分割日志并命名 yyyymmdd
    tm* tm_val = systemtime::get_local_time();
    return static_cast<uint64_t>(tm_val->tm_year + 1900) * 10000 + static_cast<uint64_t>(tm_val->tm_mon + 1) * 100 +
           static_cast<uint64_t>(tm_val->tm_mday);
}

uint64_t file_appender::get_hours_suffix()
{
    // 按小时分割日志并命名 yyyymmddhh
    tm* tm_val = systemtime::get_local_time();
    return static_cast<uint64_t>(tm_val->tm_year + 1900) * 1000000 + static_cast<uint64_t>(tm_val->tm_mon + 1) * 10000 +
           static_cast<uint64_t>(tm_val->tm_mday) * 100 + static_cast<uint64_t>(tm_val->tm_hour);
}

void file_appender::log(LOG_LEVEL level, log_event* event)
{
    _formatter->format(level, event);
}

logger::logger()
{
    _root_appender = new file_appender("/home/cassobee/debug/logdir", "trace");
}

void logger::log(LOG_LEVEL level, log_event* event)
{
    _root_appender->log(level, event);
}

void log_manager::init()
{
    _root_logger = new logger;
    _eventpool.init(LOG_EVENT_POOLSIZE);
}

void log_manager::log(LOG_LEVEL level, const char* fmt, ...)
{
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    _root_logger->log(level, std::string(buf, n));
}

}
