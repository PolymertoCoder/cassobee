#include "log_manager.h"
#include "log_appender.h"
#include "logger.h"
#include "config.h"
#include "remotelog.h"

namespace cassobee
{

void log_manager::init()
{
    auto cfg = config::get_instance();
    std::string logdir   = cfg->get("log", "dir");
    std::string filename = cfg->get("log", "filename");

    bool asynclog = cfg->get<bool>("log", "asynclog");
    auto loglevel = cfg->get<int>("log", "loglevel", LOG_LEVEL_TRACE);
    if(asynclog)
    {
        _file_logger = new logger((LOG_LEVEL)loglevel, new async_appender(logdir, filename));
    }
    else
    {
        _file_logger = new logger((LOG_LEVEL)loglevel, new file_appender(logdir, filename));
    }
}

void log_manager::log(LOG_LEVEL level, const log_event& evt)
{
    _file_logger->log(level, evt);
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

} // namespace cassobee