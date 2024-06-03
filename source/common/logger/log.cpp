#include "log.h"
#include "log_appender.h"
#include "systemtime.h"
#include "config.h"
#include <sstream>

namespace cassobee
{
std::string log_manager::_process_name;

void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);
    log_manager::get_instance()->log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
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
    auto cfg = config::get_instance();
    std::string logdir   = cfg->get("log", "dir");
    std::string filename = cfg->get("log", "filename");

    bool asynclog = cfg->get<bool>("log", "asynclog");
    if(asynclog)
    {
        _root_appender = new async_appender(logdir, filename);
    }
    else
    {
        _root_appender = new file_appender(logdir, filename);
    }
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
