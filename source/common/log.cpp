#include "log.h"
#include <cstdarg>

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

log_formatter::log_formatter(const std::string pattern)
    : _pattern(pattern)
{
}

template<>
void log_formatter::format<log_formatter::loglevel_fmt>(std::ostream& os)
{
    os << to_string();
}
template<>
void log_formatter::format<log_formatter::message_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::datetime_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::filename_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::fileline_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::elapse_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::threadid_fmt>(std::ostream& os)
{
    
}
template<>
void log_formatter::format<log_formatter::fiberid_fmt>(std::ostream& os)
{
    
}

std::string log_formatter::format(LOG_LEVEL level, const std::string& content)
{
    return {};
}

file_appender::file_appender(const char* logdir, const char* filename)
    : _logdir(logdir), _filename(filename)
{
}

void file_appender::log(LOG_LEVEL level, const std::string& content)
{
    
}

logger::logger()
{
    _root_appender = new file_appender("/home/cassobee/debug/logdir/", "log.h");
}

void logger::log(LOG_LEVEL level, const std::string& content)
{
    _root_appender->log(level, content);
}

void log_manager::init()
{
    _root_logger = new logger;
}

void log_manager::log(LOG_LEVEL level, const char* fmt, ...)
{
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    _root_logger->log(level, std::string(buf, n));
}

}