#include "log.h"
#include "macros.h"
#include "log_appender.h"

namespace cassobee
{

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
    //_content = content;
}

logger::logger()
{
    _root_appender = new file_appender("/home/cassobee/debug/logdir", "trace");
    //_root_appender = new async_appender("/home/cassobee/debug/logdir", "trace");
}

logger::~logger()
{
    if(_root_appender)
    {
        delete _root_appender;
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
