#pragma once

namespace bee
{
    class logclient;
}

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

enum LOG_LEVEL : unsigned char
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

// #define GLOG(loglevel, fmt, ...)
//     bee::logclient::get_instance()->glog(loglevel, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)

// #define TRACELOG(fmt, ...) GLOG(LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
// #define DEBUGLOG(fmt, ...) GLOG(LOG_LEVEL::DEBUG, fmt, ##__VA_ARGS__)
// #define INFOLOG(fmt,  ...) GLOG(LOG_LEVEL::INFO,  fmt, ##__VA_ARGS__)
// #define WARNLOG(fmt,  ...) GLOG(LOG_LEVEL::WARN,  fmt, ##__VA_ARGS__)
// #define ERRORLOG(fmt, ...) GLOG(LOG_LEVEL::ERROR, fmt, ##__VA_ARGS__)
// #define FATALLOG(fmt, ...) GLOG(LOG_LEVEL::FATAL, fmt, ##__VA_ARGS__)

#define TRACELOG GLOG(LOG_LEVEL::TRACE, __FILENAME__, __LINE__)
#define DEBUGLOG GLOG(LOG_LEVEL::DEBUG, __FILENAME__, __LINE__)
#define INFOLOG  GLOG(LOG_LEVEL::INFO,  __FILENAME__, __LINE__)
#define WARNLOG  GLOG(LOG_LEVEL::WARN,  __FILENAME__, __LINE__)
#define ERRORLOG GLOG(LOG_LEVEL::ERROR, __FILENAME__, __LINE__)
#define FATALLOG GLOG(LOG_LEVEL::FATAL, __FILENAME__, __LINE__)

#define TRACELOGF(fmt, ...) GLOGF(LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define DEBUGLOGF(fmt, ...) GLOGF(LOG_LEVEL::DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOGF(fmt, ...)  GLOGF(LOG_LEVEL::INFO,  fmt, ##__VA_ARGS__)
#define WARNLOGF(fmt, ...)  GLOGF(LOG_LEVEL::WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOGF(fmt, ...) GLOGF(LOG_LEVEL::ERROR, fmt, ##__VA_ARGS__)
#define FATALLOGF(fmt, ...) GLOGF(LOG_LEVEL::FATAL, fmt, ##__VA_ARGS__)

#define local_log(fmt, ...) \
    bee::logclient::get_instance()->console_log(LOG_LEVEL::INFO, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
