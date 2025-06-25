#pragma once
#include "types.h"
#include "format.h"
#include "log.h"
#include "logger.h"
#include <cstdarg>

namespace bee
{
class logger;
class log_event;
class influxlog_event;
class logserver_manager;

extern thread_local bee::ostringstream g_logstream;

class logclient : public singleton_support<logclient>
{
public:
    template<LOG_OUTPUT output> struct impl;

    void init();
    void glog(LOG_LEVEL level, const char* filename, int line, std::string content);
    void console_log(LOG_LEVEL level, const char* filename, int line, std::string content);
    void influx_log(const std::string& measurement, const std::map<std::string, std::string> tags, const std::map<std::string, std::string>& fields, TIMETYPE timestamp/*ns*/);

    FORCE_INLINE logger* get_console_logger() { return _console_logger; }
    void set_process_name(const std::string& process_name);
    void set_logserver(logserver_manager* logserver);
    void commit_log(LOG_LEVEL level, const log_event& event);
    void commit_influxlog(const influxlog_event& event);

private:
    bool _is_logserver = false;
    LOG_LEVEL _loglevel;
    std::string _process_name;
    logserver_manager* _logserver;
    logger* _console_logger = nullptr;
};

template<LOG_OUTPUT output>
struct logclient::impl
{
    impl(LOG_LEVEL level, const char* filename, int line)
        : _loglevel(level), _line(line), _filename(filename) {}

    ~impl()
    {
        if(!g_logstream.empty())
        {
            g_logstream.truncate(LOG_BUFFER_SIZE);
            log(g_logstream.str());
            g_logstream.clear();
        }
    }

    void log(std::string&& content)
    {
        if constexpr(output == LOG_OUTPUT::CONSOLE)
        {
            logclient::get_instance()->console_log(_loglevel, _filename, _line, std::move(content));
        }
        else if constexpr(output == LOG_OUTPUT::LOGFILE)
        {
            logclient::get_instance()->glog(_loglevel, _filename, _line,  std::move(content));
        }
    }

    void operator()(const char* fmt, ...) FORMAT_PRINT_CHECK(2, 3)
    {
        thread_local char content[LOG_BUFFER_SIZE];
        va_list args;
        va_start(args, fmt);
        vsnprintf(content, sizeof(content), fmt, args);
        va_end(args);
        log(std::string(content));
    }

    void operator()(std::string content)
    {
        log(std::move(content));
    }

    template<typename T> impl& operator<<(T&& arg)
    {
        if(g_logstream.size() < LOG_BUFFER_SIZE)
        {
            g_logstream << std::forward<T>(arg);
        }
        return *this;
    }

    LOG_LEVEL _loglevel;
    int _line;
    const char* _filename;
};

template<>
struct logclient::impl<INFLUX>
{

};

using FILE_GLOG = logclient::impl<LOGFILE>;
using CONSOLE_GLOG = logclient::impl<CONSOLE>;
using INFLUX_GLOG = logclient::impl<INFLUX>;

#define FILE_GLOGF(loglevel, fmt, ...) \
    FILE_GLOG(loglevel, __FILENAME__, __LINE__)(std::format(fmt, ##__VA_ARGS__))

#define CONSOLE_GLOGF(loglevel, fmt, ...) \
    CONSOLE_GLOG(loglevel, __FILENAME__, __LINE__)(std::format(fmt, ##__VA_ARGS__))

} // namespace bee