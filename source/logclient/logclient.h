#pragma once
#include <string>
#include "common.h"
#include "types.h"
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
    void glog(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...) FORMAT_PRINT_CHECK(8, 9);
    void console_log(LOG_LEVEL level, const char* filename, int line, int threadid, int fiberid, std::string elapse, const char* fmt, ...) FORMAT_PRINT_CHECK(8, 9);

    FORCE_INLINE logger* get_console_logger() { return _console_logger; }
    void set_process_name(const std::string& process_name);
    void set_logserver(logserver_manager* logserver);
    void send(remotelog& remotelog);

private:
    bool _is_logserver = false;
    LOG_LEVEL _loglevel;
    std::string _process_name;
    logserver_manager* _logserver;
    logger* _console_logger = nullptr;
};

} // namespace bee