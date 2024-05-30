#pragma once
#include <cstdarg>
#include <sstream>
#include <string>
#include <unordered_map>

#include "objectpool.h"
#include "systemtime.h"
#include "types.h"
#include "util.h"
#include "macros.h"

#define GLOG(loglevel, fmt, ...) \
    cassobee::glog(loglevel, __FILE__, __LINE__, gettid(), 0, std::to_string(get_process_elapse()), fmt, __VA_ARGS__)

#define DEBUGLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_DEBUG, fmt, __VA_ARGS__)
#define INFOLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_INFO,  fmt, __VA_ARGS__)
#define WARNLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_WARN,  fmt, __VA_ARGS__)
#define ERRORLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_ERROR, fmt, __VA_ARGS__)
#define FATALLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_FATAL, fmt, __VA_ARGS__)

namespace cassobee
{

class log_appender;
class logger;
class log_manager;

enum LOG_LEVEL
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...);

std::string to_string(LOG_LEVEL level);

class log_event
{
public:
    log_event() = default;
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
    int _line = 0;              // 行号
    TIMETYPE _time = 0;         // 时间戳
    int _threadid = 0;          // 线程id
    int _fiberid = 0;           // 协程id
    std::string _elapse;        // 程序从启动到现在的毫秒数
    std::stringstream _content; // 日志内容
};

inline void test()
{
    std::string content;
    log_event* evt = new log_event(__FILE__, __LINE__, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), std::move(content));
    UNUSE(evt);
}

class logger
{
public:
    logger();
    ~logger();
    void log(LOG_LEVEL level, log_event* event);

    void add_appender(const std::string& name, log_appender* appender);
    void del_appender(const std::string& name);
    void clr_appender();
private:
    log_appender*  _root_appender  = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

#define LOG_EVENT_POOLSIZE 1024

class log_manager : public singleton_support<log_manager>
{
public:
    void init();
    template<typename ...params_type>
    void log(LOG_LEVEL level, params_type... params)
    {
        auto [id, evt] = _eventpool.alloc();
        evt->assign(params...);
        _root_logger->log(level, evt);
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