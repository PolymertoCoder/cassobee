#include "log.h"
#include "systemtime.h"
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

class message_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_content();
    }
};

class loglevel_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << to_string(level);
    }
};

class elapse_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_elapse();
    }
};

class threadid_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_threadid();
    }
};

class fiberid_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_fiberid();
    }
};

class datetime_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << systemtime::format_time(event->get_time());
    }
};

class filename_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_filename();
    }
};

class line_format_item : public log_formatter::format_item
{
public:
    void format(std::ostream& os, LOG_LEVEL level, log_event* event) override
    {
        os << event->get_line();
    }
};

class newline_format_item : public log_formatter::format_item
{
public:
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
    std::vector<std::tuple<std::string, std::string, int> > vec;
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
            if(_pattern[i + 1] == '%')
            {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < _pattern.size())
        {
            if(!fmt_status && (!isalpha(_pattern[n]) && _pattern[n] != '{' && _pattern[n] != '}'))
            { 
                str = _pattern.substr(i + 1, n - i - 1);
                break; 
            }
            if(fmt_status == 0)
            {
                if(_pattern[n] == '{')
                {
                    str = _pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; // 解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }     
            else if(fmt_status == 1)
            {
                if(_pattern[n] == '}')
                {
                    fmt = _pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == _pattern.size())
            {
                if(str.empty())
                {
                    str = _pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0)
        {
            if(!nstr.empty())
            {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if(fmt_status == 1)
        {
            printf("pattern parse error: %s-%s\n", _pattern.data(), _pattern.substr(i).data());
            _error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0)); 
        }
    }

    if(!nstr.empty())
    {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<format_item(const std::string&)> > format_items = {
#define XX(str, C) \
        { #str, [](const std::string& fmt) { return (format_item*)(new C(fmt)); } }
    
    XX(m, message_format_item),
    XX(p, loglevel_format_item),
    XX(r, elapse_format_item),
    XX(t, threadid_format_item),
    XX(n, newline_format_item),
    XX(d, datetime_format_item),
    XX(f, filename_format_item),
    XX(l, line_format_item),
    XX(T, tab_format_item),
    XX(F, fiberid_format_item),
#undef XX
    };

    for(auto& i : vec)
    {
        if(std::get<2>(i) == 0)
        {
            _items.push_back(new string_format_item(std::get<0>(i)));
        }
        else
        {
            auto it = format_items.find(std::get<0>(i));
            if(it == format_items.end())
            {
                _items.push_back(new string_format_item("<<error_format %" + std::get<0>(i) + ">>"));
                _error = true;
            }
            else
            {
                _items.push_back(it->second(std::get<1>(i)));
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << "-" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std:: cout << m_items.size() << std::endl;
}



std::string log_formatter::format(LOG_LEVEL level, const std::string& content)
{
    return {};
}

file_appender::file_appender(const char* logdir, const char* filename)
    : _logdir(logdir), _filename(filename)
{
}

void file_appender::log(LOG_LEVEL level, const std::string& content)
{
    
}

logger::logger()
{
    _root_appender = new file_appender("/home/cassobee/debug/logdir/", "log.h");
}

void logger::log(LOG_LEVEL level, const std::string& content)
{
    _root_appender->log(level, content);
}

void log_manager::init()
{
    _root_logger = new logger;
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
