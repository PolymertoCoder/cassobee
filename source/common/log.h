#pragma once
#include "objectpool.h"
#include "systemtime.h"
#include "types.h"
#include "util.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define LOG(loglevel, fmt, ...) \
    log_manager::get_instance()->log_event

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
    log_event();
    log_event(std::string filename, int line, TIMETYPE time, int threadid, int fiberid, std::string elapse, std::string content)
        : _filename(std::move(filename)), _line(line), _time(time)
        , _threadid(threadid), _fiberid(fiberid), _elapse(std::move(elapse))
        , _content(std::move(content)) {}
    void assign(std::string filename, int line, TIMETYPE time, int threadid, int fiberid, std::string elapse, std::string content);
    FORCE_INLINE std::string get_filename() { return _filename; }
    FORCE_INLINE int get_line() { return _line; }
    FORCE_INLINE TIMETYPE get_time() { return _time; }
    FORCE_INLINE std::string get_content() { return _content.str(); }
    FORCE_INLINE std::string get_elapse() { return _elapse; }
    FORCE_INLINE int get_threadid() { return _threadid; }
    FORCE_INLINE int get_fiberid() { return _fiberid; }

    FORCE_INLINE std::stringstream& get_stream() { return _content; }
    
private:
    std::string _filename;      // 文件名
    int _line;                  // 行号
    TIMETYPE _time;             // 时间戳
    int _threadid;              // 线程id
    int _fiberid;               // 协程id
    std::string _elapse;        // 程序从启动到现在的毫秒数
    std::stringstream _content; // 日志内容
};

inline void test()
{
    std::string content;
    log_event* evt = new log_event(__FILE__, __LINE__, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), std::move(content));
}

class log_formatter
{
public:
    log_formatter(const std::string pattern);
    std::string format(LOG_LEVEL level, log_event* event);
    class format_item
    {
    public:
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

class file_appender : public log_appender
{
public:
    file_appender(std::string logdir, std::string filename);
    bool init();
    static uint64_t get_days_suffix();
    static uint64_t get_hours_suffix();
    virtual void log(LOG_LEVEL level, log_event* event) override;
private:
    std::string  _filedir;
    std::string  _filename;
    std::string  _filepath;
    std::fstream _filestream;
    TIMETYPE _now_suffix_time;
};

class logger
{
public:
    logger();
    void log(LOG_LEVEL level, log_event* event);

    void add_appender(const std::string& name, log_appender* appender);
    void del_appender(const std::string& name);
    void clr_appender();
private:
    log_appender*  _root_appender  = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

#define LOG_EVENT_POOLSIZE 1024

class log_manager : public singleton_support<logger>
{
public:
    void init();
    void log(LOG_LEVEL level, const char* fmt, ...);
    template<typename ...params_type>
    void log(params_type... params)
    {
        _eventpool.alloc();
    }

    void get_logger(std::string name);
    void add_logger(std::string name, logger* logger);
    void del_logger(std::string name);
private:
    logger* _root_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
    objectpool<log_event> _eventpool;
};

}