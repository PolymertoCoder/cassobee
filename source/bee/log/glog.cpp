#include "glog.h"

#include <cstdio>
#include "glog.h"
#include "log_appender.h"
#include "logger.h"
#include "config.h"
#include "log_event.h"
#include "influxlog_event.h"
#include "systemtime.h"

namespace bee
{
thread_local bee::log_event g_logevent;
thread_local bee::influxlog_event g_influxlogevent;
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
    if(!_console_logger) return;
    g_logevent.set_process_name(_process_name);
    g_logevent.set_filename(filename);
    g_logevent.set_line(line);
    g_logevent.set_timestamp(systemtime::get_time());
    g_logevent.set_threadid(gettid());
    g_logevent.set_elapse(std::to_string(get_process_elapse()));
    g_logevent.set_content(std::move(content));

    _console_logger->log(level, g_logevent);
}

void logclient::influx_log(const std::string& measurement, const std::map<std::string, std::string> tags, const std::map<std::string, std::string>& fields, TIMETYPE timestamp)
{
    g_influxlogevent.measurement = measurement;
    g_influxlogevent.tags = tags;
    g_influxlogevent.fields = fields;
    g_influxlogevent.timestamp = timestamp;

    commit_influxlog(g_influxlogevent);
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
    if(!_console_logger) return;
    _console_logger->log(level, event);
}

ATTR_WEAK void commit_influxlog(const influxlog_event& event)
{
}

} // namespace bee