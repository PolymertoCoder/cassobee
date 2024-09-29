#include <cstdarg>
#include <cstdio>
#include "log.h"
#include "config.h"
#include "logger.h"
#include "systemtime.h"
#include "log_appender.h"

#include "remotelog.h"

namespace cassobee
{

void logclient::init()
{
    auto cfg = config::get_instance();
    _loglevel = cfg->get<LOG_LEVEL>("log", "loglevel", LOG_LEVEL_TRACE);

    _console_logger = new logger(_loglevel, new console_appender());
    size_t poolsize = cfg->get<int>("log", "poolsize");
    _eventpool.init(poolsize);
}

void logclient::glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    if(level < _loglevel) return;
    thread_local char content[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);

    auto [id, evt] = _eventpool.alloc();
    evt->set_process_name(_process_name);
    evt->set_filename(filename);
    evt->set_line(line);
    evt->set_timestamp(systemtime::get_time());
    evt->set_threadid(gettid());
    evt->set_fiberid(0);
    evt->set_elapse(std::to_string(get_process_elapse()));
    evt->set_content(content);
    send(remotelog(level, *evt));
}

void logclient::console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);

    auto [id, evt] = _eventpool.alloc();
    evt->set_process_name(_process_name);
    evt->set_filename(filename);
    evt->set_line(line);
    evt->set_timestamp(systemtime::get_time());
    evt->set_threadid(gettid());
    evt->set_fiberid(0);
    evt->set_elapse(std::to_string(get_process_elapse()));
    evt->set_content(content);
    _console_logger->log(level, *evt);
    _eventpool.free(id);
}

void logclient::send(const protocol& prot)
{
    _logserver->send_protocol(_logsever_sid, prot);
}

}
