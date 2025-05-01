#include "glog.h"

#include <cstdarg>
#include <cstdio>
#include "glog.h"
#include "log_appender.h"
#include "logger.h"
#include "config.h"
#include "log_event.h"
#include "systemtime.h"

namespace bee
{
thread_local bee::log_event g_logevent;
thread_local bee::ostringstream g_logstream;

void logclient::init()
{
    auto cfg = config::get_instance();
    _loglevel = cfg->get<LOG_LEVEL>("log", "loglevel", LOG_LEVEL::TRACE);
    _console_logger = new logger(_loglevel, new console_appender());
    g_logstream.reserve(LOG_BUFFER_SIZE);
}

void logclient::glog(LOG_LEVEL level, const char* filename, int line, std::string content)
{
    g_logevent.set_process_name(_process_name);
    g_logevent.set_filename(filename);
    g_logevent.set_line(line);
    g_logevent.set_timestamp(systemtime::get_time());
    g_logevent.set_threadid(gettid());
    g_logevent.set_elapse(std::to_string(get_process_elapse()));
    g_logevent.set_content(std::move(content));

    commit_log(level, g_logevent);
}

void logclient::console_log(LOG_LEVEL level, const char* filename, int line, std::string content)
{
    g_logevent.set_process_name(_process_name);
    g_logevent.set_filename(filename);
    g_logevent.set_line(line);
    g_logevent.set_timestamp(systemtime::get_time());
    g_logevent.set_threadid(gettid());
    g_logevent.set_elapse(std::to_string(get_process_elapse()));
    g_logevent.set_content(std::move(content));

    _console_logger->log(level, g_logevent);
}

logclient::impl::impl(LOG_OUTPUT output, LOG_LEVEL level, const char* filename, int line)
    : _output(output), _loglevel(level), _line(line), _filename(filename) {}

logclient::impl::~impl()
{
    if(!g_logstream.empty())
    {
        g_logstream.truncate(LOG_BUFFER_SIZE);
        log(g_logstream.str());
        g_logstream.clear();
    }
}

void logclient::impl::log(std::string&& content)
{
    if(_output == LOG_OUTPUT::CONSOLE)
    {
        logclient::get_instance()->console_log(_loglevel, _filename, _line, std::move(content));
    }
    else if(_output == LOG_OUTPUT::LOGFILE)
    {
        logclient::get_instance()->glog(_loglevel, _filename, _line,  std::move(content));
    }
}

void logclient::impl::operator()(std::string content)
{
    log(std::move(content));
}

void logclient::impl::operator()(const char* fmt, ...)
{
    thread_local char content[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content), fmt, args);
    va_end(args);
    log(std::string(content));
}

void logclient::set_process_name(const std::string& process_name)
{
    if(process_name == std::string("logserver"))
    {
        _is_logserver = true;
        printf("i am logserver!!!\n");
    }
    _process_name = process_name;
}

ATTR_WEAK void logclient::set_logserver(logserver_manager* logserver)
{
}

ATTR_WEAK void logclient::commit_log(LOG_LEVEL level, const log_event& event)
{
    _console_logger->log(level, event);
}

} // namespace bee