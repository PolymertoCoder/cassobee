#pragma once
#include <stddef.h>
#include <condition_variable>
#include <fstream>
#include <string>
#include <thread>
#ifdef _REENTRANT
#include <atomic>
#endif

#include "glog.h"
#include "ring_buffer.h"
#include "types.h"
#include "lock.h"


namespace bee
{
class log_event;
class log_formatter;
class log_rotator;

// 日志输出器
class log_appender
{
public:
    log_appender();
    virtual ~log_appender() = default;
    virtual void log(const std::string& content) = 0; // 忽略formatter直接输出
    virtual void log(LOG_LEVEL level, const log_event& event) = 0;

protected:
    bee::mutex _locker;
    log_formatter* _formatter = nullptr;
};

// 日志输出到终端
class console_appender : public log_appender
{
public:
    virtual void log(const std::string& content) override;
    virtual void log(LOG_LEVEL level, const log_event& event) override;
};

// 支持日志流转功能的appender
class rotatable_log_appender : public log_appender
{
public:
    rotatable_log_appender(log_rotator* rotator);
    virtual ~rotatable_log_appender();
    virtual bool rotate() = 0;
    std::string get_pre_suffix();
    std::string get_suffix();

private:
    log_rotator* _rotator; 
};

// 日志输出到文件
class file_appender : public rotatable_log_appender
{
public:
    file_appender(std::string logdir, std::string filename);
    ~file_appender();
    virtual bool rotate() override;
    virtual void log(const std::string& content) override;
    virtual void log(LOG_LEVEL level, const log_event& event) override;

    FORCE_INLINE std::string get_filepath() const { return _filepath; }

protected:
    virtual bool reopen();

protected:
    std::string  _filedir;
    std::string  _filename;
    std::string  _filepath;
    std::fstream _filestream;
};

class influxlog_appender : public file_appender
{
public:
    influxlog_appender(std::string logdir, std::string filename);

protected:
    virtual bool reopen() override;
};

// 异步日志输出器
class async_appender : public file_appender
{
public:
    async_appender(std::string logdir, std::string filename);
    ~async_appender();
    virtual void log(const std::string& content) override;
    virtual void log(LOG_LEVEL level, const log_event& event) override;

    void start();
    void stop();

private:
    using buffer_type = bee::ring_buffer<4096*128>;
    TIMETYPE _timeout = 0; // ms
    size_t   _threshold = 0;
#ifdef _REENTRANT
    std::atomic_bool _running = false;
#else
    bool _running = false;
#endif
    std::condition_variable_any _cond;
    std::thread* _thread = nullptr;
    buffer_type _buf;
};

}