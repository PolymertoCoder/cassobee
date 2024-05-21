#pragma once
#include "types.h"
#include "util.h"
#include <string>
#include <unordered_map>

namespace cassobee
{

enum LOG_LEVEL
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

std::string to_string(LOG_LEVEL level);

class log_formatter
{
public:
    log_formatter(const std::string pattern);
    std::string format(LOG_LEVEL level, const std::string& content);
    class formatter_item
    {
    public:
        virtual void format(std::ostream& os) = 0;
    };

private:
    std::string _pattern;
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
    log_appender* _root_appender = nullptr;
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