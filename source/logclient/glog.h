#pragma once
#include <format>
#include <print>
#include <string>
#include "common.h"
#include "log_event.h"
#include "types.h"
#include "format.h"
#include "log.h"
#include "logserver_manager.h"

namespace bee
{
class logger;
class remotelog;

class logclient : public singleton_support<logclient>
{
public:
    void init();
    void glog(LOG_LEVEL level, const char* filename, int line, std::string content);
    void console_log(LOG_LEVEL level, const char* filename, int line, std::string content);

    template<typename... Args>
    void glog(LOG_LEVEL level, const char* filename, int line, std::format_string<Args...> fmt, Args&&... args)
    {
        if(level < _loglevel) return;
        auto formatargs = std::make_format_args(args...);
        glog(level, filename, line, std::vformat(fmt.get(), formatargs));
    }

    template<typename... Args>
    void console_log(LOG_LEVEL level, const char* filename, int line, std::format_string<Args...> fmt, Args&&... args)
    {
        if(level < _loglevel) return;
        auto formatargs = std::make_format_args(args...);
        console_log(level, filename, line, std::vformat(fmt.get(), formatargs));
    }

    // 流式日志
    void start_logstream(LOG_LEVEL level, const char* filename, int line);
    void end_logstream();

    template<typename T> logclient& operator<<(T&& arg)
    {
        _logstream_param.content_buf << std::forward<T>(arg);
        return *this;
    }

    FORCE_INLINE logger* get_console_logger() { return _console_logger; }
    void set_process_name(const std::string& process_name);
    void set_logserver(logserver_manager* logserver);
    void send(remotelog& remotelog);

public:
    struct support
    {
        support(LOG_LEVEL level, const char* filename, int line)
            : _logclient(logclient::get_instance()), _loglevel(level), _filename(filename), _line(line) {}
        
        ~support() { if(_is_logstream) _logclient->end_logstream(); }
    
        template<typename... Args>
        void operator()(std::format_string<Args...> fmt, Args&&... args)
        {
            _logclient->glog(_loglevel, _filename, _line, fmt, std::forward<Args>(args)...);
        }
    
        template<typename T>
        logclient& operator<<(T&& arg)
        {
            _is_logstream = true;
            _logclient->start_logstream(_loglevel, _filename, _line);
            return *_logclient << std::forward<T>(arg);
        }

        bool _is_logstream = false;
        logclient*  _logclient;
        LOG_LEVEL   _loglevel;
        const char* _filename;
        int _line;
    };

private:
    bool _is_logserver = false;
    LOG_LEVEL _loglevel;
    std::string _process_name;
    struct
    {
        LOG_LEVEL loglevel;
        const char* filename;
        int line;
        bee::ostringstream content_buf;
    } _logstream_param;

    logserver_manager* _logserver;
    logger* _console_logger = nullptr;
};

using GLOG = logclient::support;

} // namespace bee