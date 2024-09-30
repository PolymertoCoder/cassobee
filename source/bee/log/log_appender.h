#pragma once
#include <stddef.h>
#include <condition_variable>
#include <fstream>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "log.h"
#include "ring_buffer.h"
#include "types.h"


namespace cassobee
{
class log_event;
class log_formatter;

// 日志输出器
class log_appender
{
public:
    log_appender();
    virtual ~log_appender() = default;
    virtual void log(LOG_LEVEL level, const log_event& event) = 0;

protected:
    std::mutex _locker;
    log_formatter* _formatter = nullptr;
};

// 日志输出到终端
class console_appender : public log_appender
{
public:
    virtual void log(LOG_LEVEL level, const log_event& event) override;
};

// 负责日志的流转功能
class rotate_log_support
{
public:
    virtual ~rotate_log_support() = default;
    virtual bool is_rotate() = 0;
    FORCE_INLINE const char* get_suffix() { return _suffix.c_str(); }

protected:
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
    virtual ~time_rotater() = default;
    bool update(TIMETYPE curtime);
    virtual bool is_rotate() override;

private:
    ROTATE_TYPE _rotate_type;
    TIMETYPE    _next_rotate_time = 0; // 下一次分割日志的时间
};

// 日志输出到文件
class file_appender : public log_appender
{
public:
    file_appender(std::string logdir, std::string filename);
    ~file_appender();
    virtual void log(LOG_LEVEL level, const log_event& event) override;

protected:
    bool reopen();

protected:
    std::atomic_bool _running = false;
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
    ~async_appender();
    virtual void log(LOG_LEVEL level, const log_event& event) override;

    void start();
    void stop();

private:
    template<size_t N> using buffer_type = cassobee::ring_buffer<N>;
    TIMETYPE _timeout = 0; // ms
    size_t   _threshold = 0;
    std::condition_variable _cond;
    std::thread* _thread = nullptr;
    buffer_type<4096*8> _buf;
};

}