#pragma once
#include <string>
#include "common.h"
#include "objectpool.h"
#include "session_manager.h"
#include "types.h"
#include "log_event.h"

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define GLOG(loglevel, fmt, ...) \
    cassobee::logclient::get_instance()->glog(loglevel, __FILENAME__, __LINE__, gettid(), 0, {}, fmt, ##__VA_ARGS__)

#define TRACELOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define DEBUGLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define INFOLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define WARNLOG(fmt,  ...) GLOG(cassobee::LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define ERRORLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define FATALLOG(fmt, ...) GLOG(cassobee::LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

#define local_log(fmt, ...) \
    cassobee::logclient::get_instance()->console_log(cassobee::LOG_LEVEL_INFO, __FILENAME__, __LINE__, gettid(), 0, {}, fmt, ##__VA_ARGS__)

namespace cassobee
{
class logger;

enum LOG_LEVEL
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

class logclient : public singleton_support<logclient>
{
public:
    void init();
    void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...) FORMAT_PRINT_CHECK(8, 9);
    void console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...) FORMAT_PRINT_CHECK(8, 9);
    void send(const protocol& prot);

private:
    std::string _process_name;
    SID _logsever_sid;
    session_manager* _logserver;
    LOG_LEVEL _loglevel;
    logger* _console_logger = nullptr;
    objectpool<log_event> _eventpool;
};

}