#pragma once

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG_BUFFER_SIZE 2048

enum LOG_OUTPUT : unsigned char
{
    CONSOLE,
    LOGFILE,
    INFLUX,
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

#define local_log CONSOLE_GLOG(LOG_LEVEL::TRACE, __FILENAME__, __LINE__)
#define local_log_f(fmt, ...) CONSOLE_GLOGF(LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)

// printf风格的格式化
#define TRACELOG FILE_GLOG(LOG_LEVEL::TRACE, __FILENAME__, __LINE__)
#define DEBUGLOG FILE_GLOG(LOG_LEVEL::DEBUG, __FILENAME__, __LINE__)
#define INFOLOG  FILE_GLOG(LOG_LEVEL::INFO,  __FILENAME__, __LINE__)
#define WARNLOG  FILE_GLOG(LOG_LEVEL::WARN,  __FILENAME__, __LINE__)
#define ERRORLOG FILE_GLOG(LOG_LEVEL::ERROR, __FILENAME__, __LINE__)
#define FATALLOG FILE_GLOG(LOG_LEVEL::FATAL, __FILENAME__, __LINE__)

// std::format风格的格式化
#define TRACELOGF(fmt, ...) FILE_GLOGF(LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define DEBUGLOGF(fmt, ...) FILE_GLOGF(LOG_LEVEL::DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOGF(fmt, ...)  FILE_GLOGF(LOG_LEVEL::INFO,  fmt, ##__VA_ARGS__)
#define WARNLOGF(fmt, ...)  FILE_GLOGF(LOG_LEVEL::WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOGF(fmt, ...) FILE_GLOGF(LOG_LEVEL::ERROR, fmt, ##__VA_ARGS__)
#define FATALLOGF(fmt, ...) FILE_GLOGF(LOG_LEVEL::FATAL, fmt, ##__VA_ARGS__)

// InfluxDB Line Protocol Log
#define INFLUXLOG()