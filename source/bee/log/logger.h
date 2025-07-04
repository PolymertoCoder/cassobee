#pragma once
#include "glog.h"
#include <string>
#include <unordered_map>

namespace bee
{
class log_event;
class influxlog_event;
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

protected:
    LOG_LEVEL _loglevel = LOG_LEVEL::DEBUG;
    log_appender* _root_appender = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

class influx_logger : public logger
{
public:
    influx_logger(log_appender* appender);
    void influxlog(const influxlog_event& event);

protected:
    std::string escape_influx(const std::string& input);
};

} // namespace bee