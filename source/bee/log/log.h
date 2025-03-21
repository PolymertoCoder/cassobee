#pragma once

namespace bee
{
    class logclient;
}

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

enum LOG_LEVEL
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

#define GLOG(loglevel, fmt, ...) \
    bee::logclient::get_instance()->glog(loglevel, __FILENAME__, __LINE__, bee::gettid(), 0, {}, fmt, ##__VA_ARGS__)

#define TRACELOG(fmt, ...) GLOG(LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define DEBUGLOG(fmt, ...) GLOG(LOG_LEVEL::DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOG(fmt,  ...) GLOG(LOG_LEVEL::INFO,  fmt, ##__VA_ARGS__)
#define WARNLOG(fmt,  ...) GLOG(LOG_LEVEL::WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOG(fmt, ...) GLOG(LOG_LEVEL::ERROR, fmt, ##__VA_ARGS__)
#define FATALLOG(fmt, ...) GLOG(LOG_LEVEL::FATAL, fmt, ##__VA_ARGS__)

#define local_log(fmt, ...) \
    bee::logclient::get_instance()->console_log(LOG_LEVEL::INFO, __FILENAME__, __LINE__, bee::gettid(), 0, {}, fmt, ##__VA_ARGS__)
