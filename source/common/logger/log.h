#pragma once
#include <cstdarg>
#include <sstream>
#include <string>
#include <unordered_map>

#include "macros.h"
#include "objectpool.h"
#include "types.h"
#include "util.h"

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define GLOG(loglevel, fmt, ...) \
    cassobee::glog(loglevel, __FILENAME__, __LINE__, gettid(), 0, {}, fmt, ##__VA_ARGS__)

#define DEBUGLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define WARNLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define FATALLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

#define local_log(fmt, ...) \
    cassobee::console_log(cassobee::LOG_LEVEL_INFO, __FILENAME__, __LINE__, gettid(), 0, {}, fmt, ##__VA_ARGS__)

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
void console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...);

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

class logger
{
public:
    logger(log_appender* appender);
    ~logger();
    void log(LOG_LEVEL level, log_event* event);

    log_appender* get_appender(const std::string& name);
    bool add_appender(const std::string& name, log_appender* appender);
    bool del_appender(const std::string& name);
    void clr_appender();
private:
    log_appender* _root_appender  = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

class log_manager : public singleton_support<log_manager>
{
public:
    void init();

    template<typename ...params_type>
    void log(LOG_LEVEL level, params_type... params)
    {
        auto [id, evt] = _eventpool.alloc();
        if(PREDICT_FALSE(!evt))
        {
            printf("logevent pool is full or not initialize\n");
            return;
        }
        evt->assign(params...);
        _file_logger->log(level, evt);
        _eventpool.free(id);
    }
    template<typename ...params_type>
    void console_log(LOG_LEVEL level, params_type... params)
    {
        auto [id, evt] = _eventpool.alloc();
        if(PREDICT_FALSE(!evt))
        {
            printf("logevent pool is full or not initialize\n");
            return;
        }
        evt->assign(params...);
        _console_logger->log(level, evt);
        _eventpool.free(id);
    }

    logger* get_logger(std::string name);
    bool add_logger(std::string name, logger* logger);
    bool del_logger(std::string name);

    static void set_process_name(const char* name) { _process_name = name; }
    static const char* get_process_name() { return _process_name.data(); }
    
private:
    static std::string _process_name;
    logger* _file_logger = nullptr;
    logger* _console_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
    objectpool<log_event> _eventpool;
};

}