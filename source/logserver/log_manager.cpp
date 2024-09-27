#include "log_manager.h"
#include "log_appender.h"
#include "config.h"

namespace cassobee
{

std::string log_manager::_process_name;

logger::logger(LOG_LEVEL level, log_appender* appender)
    : _loglevel(level), _root_appender(appender)
{
}

logger::~logger()
{
    delete _root_appender;
    _root_appender = nullptr;
    clr_appender();
}

void logger::log(LOG_LEVEL level, log_event* event)
{
    //if(level >= _loglevel) return;
    _root_appender->log(level, event);
}

log_appender* logger::get_appender(const std::string& name)
{
    auto iter = _appenders.find(name);
    return iter != _appenders.end() ? iter->second : nullptr;
}

bool logger::add_appender(const std::string& name, log_appender* appender)
{
    return _appenders.emplace(name, appender).second;
}

bool logger::del_appender(const std::string& name)
{
    if(auto iter = _appenders.find(name); iter != _appenders.end())
    {
        delete iter->second;
        return true;
    }
    return false;
}

void logger::clr_appender()
{
    for(auto& [name, appender] : _appenders)
    {
        delete appender;
        appender = nullptr;
    }
    _appenders.clear();
}

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

    _console_logger = new logger((LOG_LEVEL)loglevel, new console_appender());

    size_t poolsize = cfg->get<int>("log", "poolsize");
    _eventpool.init(poolsize);
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

} // namespace cassobee