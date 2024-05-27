#pragma once
#include <fstream>
#include "log.h"
#include "lock.h"
#include "log_formatter.h"

namespace cassobee
{

// 日志输出器
class log_appender
{
public:
    virtual void log(LOG_LEVEL level, log_event* event) = 0;
    FORCE_INLINE void set_formatter(log_formatter* formatter) { _formatter = formatter; }
protected:
    log_formatter* _formatter;
};

class console_appender : public log_appender
{
public:
    virtual void log(LOG_LEVEL level, log_event* event) override;
};

// 负责日志的流转功能
class rotate_log_support
{
public:
    virtual bool is_rotate() = 0;
    FORCE_INLINE const char* get_suffix() { return _suffix.c_str(); }
private:
    std::string _suffix;
};

// 根据时间流转日志
class time_rotater : public rotate_log_support
{
public:
    enum ROTATE_TYPE
    {
        ROTATE_TYPE_HOUR,
        ROTATE_TYPE_DAY,
    };
    time_rotater(ROTATE_TYPE rotate_type);
    virtual bool is_rotate() override;
private:
    ROTATE_TYPE _rotate_type;
    TIMETYPE    _next_cut_time; // 下一次分割日志的时间
};

class file_appender : public log_appender
{
public:
    file_appender(std::string logdir, std::string filename);
    ~file_appender();
    bool init();
    
    virtual void log(LOG_LEVEL level, log_event* event) override;
private:
    cassobee::mutex     _locker;
    rotate_log_support* _rotater; 

    std::string  _filedir;
    std::string  _filename;
    std::string  _filepath;
    std::fstream _filestream;
};

// 异步日志输出器
class async_appender : public file_appender
{
public:
    async_appender(std::string logdir, std::string filename);
    virtual void log(LOG_LEVEL level, log_event* event) override;
private:
    std::thread _thread;
};

}