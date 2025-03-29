#pragma once
#include "glog.h"
#include <string>
#include <unordered_map>

namespace bee
{
class log_event;
class log_appender;

class logger
{
public:
    logger(LOG_LEVEL level, log_appender* appender);
    ~logger();
    void log(LOG_LEVEL level, const log_event& event);

    log_appender* get_appender(const std::string& name);
    bool add_appender(const std::string& name, log_appender* appender);
    bool del_appender(const std::string& name);
    void clr_appender();

private:
    LOG_LEVEL _loglevel = LOG_LEVEL::DEBUG;
    log_appender* _root_appender = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

} // namespace bee