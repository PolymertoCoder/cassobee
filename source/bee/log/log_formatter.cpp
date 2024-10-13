#include <cctype>
#include <cstdio>
#include <functional>
#include <sstream>
#include <utility>

#include "log_formatter.h"
#include "systemtime.h"
#include "macros.h"
#include "log_event.h"

namespace cassobee
{

class message_format_item : public log_formatter::format_item
{
public:
    message_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.content;
    }
};

class loglevel_format_item : public log_formatter::format_item
{
public:
    loglevel_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        switch(level)
        {
            case LOG_LEVEL_TRACE: { os << "TRACE"; } break;
            case LOG_LEVEL_DEBUG: { os << "DEBUG"; } break;
            case LOG_LEVEL_INFO:  { os << "INFO";  } break;
            case LOG_LEVEL_WARN:  { os << "WARN";  } break;
            case LOG_LEVEL_ERROR: { os << "ERROR"; } break;
            case LOG_LEVEL_FATAL: { os << "FATAL"; } break;
            default:{ os << "UNKNOWN"; } break;
        }
    }
};

class elapse_format_item : public log_formatter::format_item
{
public:
    elapse_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.elapse;
    }
};

class procname_format_item : public log_formatter::format_item
{
public:
    procname_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.process_name;
    }
};

class threadid_format_item : public log_formatter::format_item
{
public:
    threadid_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.threadid;
    }
};

class fiberid_format_item : public log_formatter::format_item
{
public:
    fiberid_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.fiberid;
    }
};

class datetime_format_item : public log_formatter::format_item
{
public:
    datetime_format_item(const std::string& fmt = "%Y-%m-%d %H:%M:%S") : _fmt(fmt) {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << systemtime::format_time(event.timestamp, _fmt.data());
    }
private:
    std::string _fmt;
};

class filename_format_item : public log_formatter::format_item
{
public:
    filename_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.filename;
    }
};

class line_format_item : public log_formatter::format_item
{
public:
    line_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << event.line;
    }
};

class newline_format_item : public log_formatter::format_item
{
public:
    newline_format_item(const std::string& str = "") {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << std::endl;
    }
};

class string_format_item : public log_formatter::format_item
{
public:
    string_format_item(const std::string& str) : _str(str) {}
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
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
    void format(std::ostream& os, LOG_LEVEL level, const log_event& event) override
    {
        os << "\t";
    }
private:
    std::string m_string;
};



/**
 * @description: 
 * @param {string} pattern
 *                 m：日志内容
 *                 p：日志等级
 *                 r：程序运行时间
 *                 c：程序名字，需要手动设置
 *                 t：线程号
 *                 n：换行符
 *                 d：时间
 *                 f：文件名
 *                 l：行号
 *                 T：tab缩进
 *                 F：协程号
 * @return {*}
 */
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
                vec.emplace_back(nstr, "", true);
                nstr.clear();
            }
            vec.emplace_back(str, fmt, false);
            i = n - 1;
        }
        else
        {
            // 格式错误，可能是缺少'}'
            printf("pattern parse error: %s-%s\n", _pattern.data(), _pattern.substr(i).data());
            _error = true;
            vec.emplace_back("<<pattern_error>>", fmt, true); 
        }
    }

    if(!nstr.empty())
    {
        vec.emplace_back(nstr, "", true);
    }
    static std::map<std::string, std::function<format_item*(const std::string&)>> format_items_creators =
    {
    #define creator(str, C) \
        { #str, [](const std::string& fmt) { return (format_item*)(new C(fmt)); } }
    
        creator(m, message_format_item),
        creator(p, loglevel_format_item),
        creator(r, elapse_format_item),
        creator(c, procname_format_item),
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
            if(auto iter = format_items_creators.find(key); iter == format_items_creators.end())
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
    printf("format_items size:%zu\n", _items.size());
}

std::string log_formatter::format(LOG_LEVEL level, const log_event& event)
{
    thread_local std::stringstream os;
    for(format_item* item : _items)
    {
        item->format(os, level, event);
    }
    return os.str();
}

}