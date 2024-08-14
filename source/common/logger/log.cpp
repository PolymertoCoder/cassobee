#include "log.h"

#include <cstdarg>

#include "log_appender.h"
#include "systemtime.h"
#include "config.h"

namespace cassobee
{
std::string log_manager::_process_name;

void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content)-1, fmt, args);
    va_end(args);
    log_manager::get_instance()->log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
}

void console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content)-1, fmt, args);
    va_end(args);
    log_manager::get_instance()->console_log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
}

void log_event::assign(std::string filename, int line, TIMETYPE time, int threadid, int fiberid, std::string elapse, std::string content)
{
    _filename = std::move(filename);
    _line = line;
    _time = time;
    _threadid = threadid;
    _fiberid = fiberid;
    _elapse.swap(elapse);
    _content.swap(content);
}

logger::logger(log_appender* appender)
    : _root_appender(appender)
{    
}

logger::~logger()
{
    delete _root_appender;
    _root_appender = nullptr;
    clr_appender();
}

void logger::log(LOG_LEVEL level, log_event* event)
{
    _root_appender->log(level, event);
}

log_appender* logger::get_appender(const std::string& name)
{
    auto iter = _appenders.find(name);
    return iter != _appenders.end() ? iter->second : nullptr;
}

bool logger::add_appender(const std::string& name, log_appender* appender)
{
    return _appenders.emplace(name, appender).second;
}

bool logger::del_appender(const std::string& name)
{
    if(auto iter = _appenders.find(name); iter != _appenders.end())
    {
        delete iter->second;
        return true;
    }
    return false;
}

void logger::clr_appender()
{
    for(auto& [name, appender] : _appenders)
    {
        delete appender;
        appender = nullptr;
    }
    _appenders.clear();
}

void log_manager::init()
{
    auto cfg = config::get_instance();
    std::string logdir   = cfg->get("log", "dir");
    std::string filename = cfg->get("log", "filename");

    bool asynclog = cfg->get<bool>("log", "asynclog");
    if(asynclog)
    {
        _file_logger = new logger(new async_appender(logdir, filename));
    }
    else
    {
        _file_logger = new logger(new file_appender(logdir, filename));
    }

    _console_logger = new logger(new console_appender());

    size_t poolsize = cfg->get<int>("log", "poolsize");
    _eventpool.init(poolsize);
}

logger* log_manager::get_logger(std::string name)
{
    auto iter = _loggers.find(name);
    return iter != _loggers.end() ? iter->second : nullptr;
}
    
bool log_manager::add_logger(std::string name, logger* logger)
{
    return _loggers.emplace(name, logger).second;
}
    
bool log_manager::del_logger(std::string name)
{
    if(auto iter = _loggers.find(name); iter != _loggers.end())
    {
        delete iter->second;
        return true;
    }
    return false;
}

}
