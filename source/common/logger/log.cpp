#include "log.h"
#include "log_appender.h"
#include <sstream>

namespace cassobee
{

void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);
    log_manager::get_instance()->log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
}

std::string to_string(LOG_LEVEL level)
{
    switch(level)
    {
        case LOG_LEVEL_DEBUG: { return "DEBUG"; } break;
        case LOG_LEVEL_INFO:  { return "INFO";  } break;
        case LOG_LEVEL_WARN:  { return "WARN";  } break;
        case LOG_LEVEL_ERROR: { return "ERROR"; } break;
        case LOG_LEVEL_FATAL: { return "FATAL"; } break;
        default:{ return "UNKNOWN"; } break;
    }
}

void log_event::assign(std::string filename, int line, TIMETYPE time, int threadid, int fiberid, std::string elapse, std::string content)
{
    _filename = std::move(filename);
    _line = line;
    _time = time;
    _threadid = threadid;
    _fiberid = fiberid;
    _elapse = std::move(elapse);
    _content = std::stringstream(std::move(content));
}

logger::logger()
{
    _root_appender = new file_appender("/home/qinchao/cassobee/debug/logdir", "trace");
    //_root_appender = new async_appender("/home/qinchao/cassobee/debug/logdir", "trace");
}

logger::~logger()
{
    if(_root_appender)
    {
        delete _root_appender;
        _root_appender = nullptr;
    }
}

void logger::log(LOG_LEVEL level, log_event* event)
{
    _root_appender->log(level, event);
}

void log_manager::init()
{
    _root_logger = new logger;
    _eventpool.init(LOG_EVENT_POOLSIZE);
}

}
