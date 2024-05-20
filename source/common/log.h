#pragma once
#include "types.h"
#include "util.h"
#include <string>
#include <unordered_map>

namespace cassobee
{

class log_event
{
public:

};

class log_formatter
{
public:
    virtual void format();
};

class log_appender
{
public:
    virtual void log() = 0;
    FORCE_INLINE void set_formatter(log_formatter* formatter) { _formatter = formatter; }
private:
    log_formatter* _formatter = nullptr;
};

class console_appender : public log_appender
{
public:
    virtual void log() override;
};

class file_appender : public log_appender
{
public:
    virtual void log() override;
private:
    std::string _logdir;
    std::string _filename;
};

class logger
{
public:
    void init();
    void log(const char* fmt, ...);
    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warn(const char* fmt, ...);
    void error(const char* fmt, ...);
    void fatal(const char* fmt, ...);

    void add_appender(log_appender* appender);
    void del_appender(log_appender* appender);
    void clr_appender();
private:
    log_appender* _root_appender = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

class log_manager : public singleton_support<logger>
{
public:

private:
    logger* _root_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
};

}