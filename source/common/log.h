#pragma once
#include "types.h"
#include "util.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace cassobee
{

class logger;

enum LOG_LEVEL
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

std::string to_string(LOG_LEVEL level);

class log_event
{
public:
    FORCE_INLINE std::string get_filename() { return _filename; }
    FORCE_INLINE int get_line() { return _line; }
    FORCE_INLINE TIMETYPE get_time() { return _time; }
    FORCE_INLINE std::string get_content() { return _content.str(); }
    FORCE_INLINE std::string get_elapse() { return _elapse; }
    FORCE_INLINE int get_threadid() { return _threadid; }
    FORCE_INLINE int get_fiberid() { return _fiberid; }

    FORCE_INLINE std::stringstream& get_stream() { return _content; }
    
private:
    std::string _filename; // 文件名
    int _line;             // 行号
    TIMETYPE _time;        // 时间戳
    std::string _elapse;   // 程序从启动到现在的毫秒数
    int _threadid;
    int _fiberid;
    std::stringstream _content;
};

class log_formatter
{
public:
    log_formatter(const std::string pattern);
    std::string format(LOG_LEVEL level, const std::string& content);
    class format_item
    {
        virtual void format(std::ostream& os, LOG_LEVEL level, log_event* event) = 0;
    };

private:
    bool _error = false;
    std::string _pattern;
    std::vector<format_item*> _items;
};

class log_appender
{
public:
    virtual void log(LOG_LEVEL level, const std::string& content) = 0;
    FORCE_INLINE void set_formatter(log_formatter* formatter) { _formatter = formatter; }
private:
    log_formatter* _formatter;
};

class console_appender : public log_appender
{
public:
    virtual void log(LOG_LEVEL level, const std::string& content) override;
};

class file_appender : public log_appender
{
public:
    file_appender(const char* logdir, const char* filename);
    virtual void log(LOG_LEVEL level, const std::string& content) override;
private:
    std::string _logdir;
    std::string _filename;
};

class logger
{
public:
    logger();
    void log(LOG_LEVEL level, const std::string& content);

    void add_appender(const std::string& name, log_appender* appender);
    void del_appender(const std::string& name);
    void clr_appender();
private:
    log_appender*  _root_appender  = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

class log_manager : public singleton_support<logger>
{
public:
    void init();
    void log(LOG_LEVEL level, const char* fmt, ...);

    void get_logger(std::string name);
    void add_logger(std::string name, logger* logger);
    void del_logger(std::string name);
private:
    logger* _root_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
};

}