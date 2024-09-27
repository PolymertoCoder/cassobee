#include <cstdarg>
#include <cstdio>
#include "log.h"
#include "remotelog.h"
#include "systemtime.h"

namespace cassobee
{

void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content)-1, fmt, args);
    va_end(args);
    log_event event(filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
    logclient::get_instance()->send(remotelog(level, event));
    // log_manager::get_instance()->log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
}

void console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...)
{
    thread_local char content[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(content, sizeof(content)-1, fmt, args);
    va_end(args);
    // log_manager::get_instance()->console_log(level, filename, line, systemtime::get_time(), gettid(), 0, std::to_string(get_process_elapse()), content);
}

void logclient::send(const protocol& prot)
{
    
}

}
