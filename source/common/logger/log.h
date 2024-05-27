#pragma once
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "objectpool.h"
#include "systemtime.h"
#include "types.h"
#include "util.h"
#include "macros.h"

#define LOG(loglevel, fmt, ...) \
    log_manager::get_instance()->log_event

namespace cassobee
{

class log_appender;
class logger;

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
    UNUSE(evt);
}

#define LOG_EVENT_POOLSIZE 1024

class log_manager : public singleton_support<logger>
{
public:
    void init();
    // template<typename ...params_type>
    // void log(LOG_LEVEL level, params_type... params)
    // {
    //     auto [id, evt] = _eventpool.alloc();
    //     evt->assign(params...);
    //     _root_logger->log(level, evt);
    // }

    void get_logger(std::string name);
    void add_logger(std::string name, logger* logger);
    void del_logger(std::string name);
private:
    logger* _root_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
    objectpool<log_event> _eventpool;
};

}