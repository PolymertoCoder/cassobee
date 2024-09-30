#include <cstdarg>
#include <cstdio>
#include "log.h"
#include "config.h"
#include "log_event.h"
#include "logger.h"
#include "systemtime.h"
#include "log_appender.h"

#include "remotelog.h"

namespace cassobee
{
thread_local cassobee::remotelog g_remotelog;
thread_local cassobee::log_event g_logevent;

void logclient::init()
{
    auto cfg = config::get_instance();
    _loglevel = cfg->get<LOG_LEVEL>("log", "loglevel", LOG_LEVEL_TRACE);
    _console_logger = new logger(_loglevel, new console_appender());
}

void logclient::glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    if(level < _loglevel) return;
    thread_local char content[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);

    g_remotelog.loglevel = level;
    g_remotelog.logevent.set_filename(filename);
    g_remotelog.logevent.set_line(line);
    g_remotelog.logevent.set_timestamp(systemtime::get_time());
    g_remotelog.logevent.set_threadid(gettid());
    g_remotelog.logevent.set_fiberid(0);
    g_remotelog.logevent.set_elapse(std::to_string(get_process_elapse()));
    g_remotelog.logevent.set_content(content);
    send(g_remotelog);
}

void logclient::console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);

    g_logevent.set_filename(filename);
    g_logevent.set_line(line);
    g_logevent.set_timestamp(systemtime::get_time());
    g_logevent.set_threadid(gettid());
    g_logevent.set_fiberid(0);
    g_logevent.set_elapse(std::to_string(get_process_elapse()));
    g_logevent.set_content(content);
    _console_logger->log(level, g_logevent);
}

void logclient::set_process_name(const std::string& process_name)
{
    if(_process_name == "logserver")
    {
        _is_logserver = true;
        printf("i am logserver!!!\n");
    }
    _process_name = process_name;
    g_remotelog.logevent.set_process_name(_process_name);
    g_logevent.set_process_name(_process_name);
}

void logclient::set_logserver(logserver_manager* logserver)
{
    _logserver = logserver;
}

void logclient::send(remotelog& remotelog)
{
    if(_is_logserver)
    {
        remotelog.run();
        return;
    }
    if(_logserver)
    {
        _logserver->send(remotelog);
    }
    else
    {
        _console_logger->log((LOG_LEVEL)remotelog.loglevel, remotelog.logevent);
    }
}

} // namespace cassobee
