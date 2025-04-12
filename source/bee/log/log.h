#pragma once

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG_BUFFER_SIZE 2048

enum LOG_OUTPUT : unsigned char
{
    CONSOLE,
    LOGFILE,
};

enum LOG_LEVEL : unsigned char
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

#define local_log  GLOG(LOG_OUTPUT::CONSOLE, LOG_LEVEL::TRACE, __FILENAME__, __LINE__)
#define local_log_f(fmt, ...) GLOGF(LOG_OUTPUT::CONSOLE, LOG_LEVEL::TRACE, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)

// printf风格的格式化
#define TRACELOG GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::TRACE, __FILENAME__, __LINE__)
#define DEBUGLOG GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::DEBUG, __FILENAME__, __LINE__)
#define INFOLOG  GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::INFO,  __FILENAME__, __LINE__)
#define WARNLOG  GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::WARN,  __FILENAME__, __LINE__)
#define ERRORLOG GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::ERROR, __FILENAME__, __LINE__)
#define FATALLOG GLOG(LOG_OUTPUT::LOGFILE, LOG_LEVEL::FATAL, __FILENAME__, __LINE__)

// std::format风格的格式化
#define TRACELOGF(fmt, ...) GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define DEBUGLOGF(fmt, ...) GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOGF(fmt, ...)  GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::INFO,  fmt, ##__VA_ARGS__)
#define WARNLOGF(fmt, ...)  GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOGF(fmt, ...) GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::ERROR, fmt, ##__VA_ARGS__)
#define FATALLOGF(fmt, ...) GLOGF(LOG_OUTPUT::LOGFILE, LOG_LEVEL::FATAL, fmt, ##__VA_ARGS__)
