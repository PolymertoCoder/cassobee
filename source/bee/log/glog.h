#pragma once
#include "types.h"
#include "format.h"
#include "log.h"
#include "logger.h"

namespace bee
{
class logger;
class log_event;
class logserver_manager;

extern thread_local bee::ostringstream g_logstream;

class logclient : public singleton_support<logclient>
{
public:
    void init();
    void glog(LOG_LEVEL level, const char* filename, int line, std::string content);
    void console_log(LOG_LEVEL level, const char* filename, int line, std::string content);

    struct impl
    {
        impl(LOG_OUTPUT output, LOG_LEVEL level, const char* filename, int line);
        ~impl();

        void log(std::string&& content);
        void operator()(const char* fmt, ...) FORMAT_PRINT_CHECK(2, 3);
        void operator()(std::string content);

        template<typename T> impl& operator<<(T&& arg)
        {
            if(g_logstream.size() < LOG_BUFFER_SIZE)
            {
                g_logstream << std::forward<T>(arg);
            }
            return *this;
        }

        LOG_OUTPUT _output;
        LOG_LEVEL _loglevel;
        int _line;
        const char* _filename;
    };

    FORCE_INLINE logger* get_console_logger() { return _console_logger; }
    void set_process_name(const std::string& process_name);
    void set_logserver(logserver_manager* logserver);
    void commit_log(LOG_LEVEL level, const log_event& event);

private:
    bool _is_logserver = false;
    LOG_LEVEL _loglevel;
    std::string _process_name;
    logserver_manager* _logserver;
    logger* _console_logger = nullptr;
};

using GLOG = logclient::impl;

#define GLOGF(logoutput, loglevel, fmt, ...) \
    GLOG(logoutput, loglevel, __FILENAME__, __LINE__)(std::format(fmt, ##__VA_ARGS__))

} // namespace bee