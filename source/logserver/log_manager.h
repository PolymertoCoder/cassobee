#pragma once
#include <unordered_map>

#include "glog.h"

namespace bee
{
class logger;
class log_event;
class influxlog_event;

class log_manager : public singleton_support<log_manager>
{
public:
    void init();
    void log(LOG_LEVEL level, const log_event& event);
    void influxlog(const influxlog_event& event);

    logger* get_logger(std::string name);
    bool add_logger(std::string name, logger* logger);
    bool del_logger(std::string name);
    
private:
    logger* _file_logger = nullptr;
    influx_logger* _influx_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
};

} // namespace bee