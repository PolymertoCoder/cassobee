#include "logger.h"
#include "log_appender.h"

namespace bee
{

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

void logger::log(LOG_LEVEL level, const log_event& event)
{
    if(level < _loglevel) return;
    _root_appender->log(level, event);
    for(const auto& [_, appender] : _appenders)
    {
        appender->log(level, event);
    }
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

} // namespace bee