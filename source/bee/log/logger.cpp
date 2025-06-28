#include "logger.h"
#include "log_appender.h"
#include "influxlog_event.h"

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

influx_logger::influx_logger(log_appender* appender)
    : logger(LOG_LEVEL::TRACE, appender)
{
}

void influx_logger::influxlog(const influxlog_event& event)
{
    thread_local bee::ostringstream line;
    line.clear();

    // 测量名称
    line << event.measurement;

    // 标签集
    for(const auto& [key, value] : event.tags)
    {
        line << ',' << escape_influx(key) << '=' << escape_influx(value);
    }
    line << ' ';
    
    // 字段集
    bool first_field = true;
    for(const auto& [key, value] : event.fields)
    {
        if(!first_field) line << ',';
        line << ',' << escape_influx(key) << '=' << escape_influx(value);
        first_field = false;
    }

    // 时间戳 (纳秒)
    line << ' ' << event.timestamp;

    _root_appender->log(line.str());
}

std::string influx_logger::escape_influx(const std::string& input)
{
    thread_local ostringstream oss;
    oss.clear();
    for(char c : input)
    {
        switch (c)
        {
            case ',':
            case ' ':
            case '=': { oss << '\\'; }
            [[fallthrough]];
            default: { oss << c; }
        }
    }
    return oss.str();
}

} // namespace bee