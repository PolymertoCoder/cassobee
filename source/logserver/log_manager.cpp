#include "log_manager.h"
#include "log_appender.h"
#include "logger.h"
#include "config.h"
#include "remotelog.h"
#include "remoteinfluxlog.h"

namespace bee
{

void log_manager::init()
{
    auto cfg = config::get_instance();
    
    {
        std::string logdir   = cfg->get("log", "dir");
        std::string filename = cfg->get("log", "filename");
        assert(logdir.size() && filename.size());
        bool asynclog = cfg->get<bool>("log", "asynclog", false);
        auto loglevel = cfg->get<int>("log", "loglevel", LOG_LEVEL::TRACE);
        if(asynclog)
        {
            _file_logger = new logger((LOG_LEVEL)loglevel, new async_appender(logdir, filename));
        }
        else
        {
            _file_logger = new logger((LOG_LEVEL)loglevel, new file_appender(logdir, filename));
        }
    }
    {
        std::string logdir   = cfg->get("influxlog", "dir");
        std::string filename = cfg->get("influxlog", "filename");
        _influx_logger = new influx_logger(new influxlog_appender(logdir, filename));
    }   
}

void log_manager::log(LOG_LEVEL level, const log_event& event)
{
    if(PREDICT_FALSE(!_file_logger))
    {
        logclient::get_instance()->get_console_logger()->log(level, event);
        return;
    }
    _file_logger->log(level, event);
    for(const auto& [_, logger] : _loggers)
    {
        logger->log(level, event);
    }
}

void log_manager::influxlog(const influxlog_event& event)
{
    if(!_influx_logger) return;
    _influx_logger->influxlog(event);
}

logger* log_manager::get_logger(std::string name)
{
    auto iter = _loggers.find(name);
    return iter != _loggers.end() ? iter->second : nullptr;
}
    
bool log_manager::add_logger(std::string name, logger* logger)
{
    return _loggers.emplace(name, logger).second;
}
    
bool log_manager::del_logger(std::string name)
{
    if(auto iter = _loggers.find(name); iter != _loggers.end())
    {
        delete iter->second;
        return true;
    }
    return false;
}

void remotelog::run()
{
    log_manager::get_instance()->log((LOG_LEVEL)loglevel, logevent);
}

void remoteinfluxlog::run()
{
    log_manager::get_instance()->influxlog(logevent);
}

} // namespace bee