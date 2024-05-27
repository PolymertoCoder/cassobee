#pragma once

#include "log.h"

namespace cassobee
{

class log_formatter
{
public:
    log_formatter(const std::string pattern);
    std::string format(LOG_LEVEL level, log_event* event);
    class format_item
    {
    public:
        virtual void format(std::ostream& os, LOG_LEVEL level, log_event* event) = 0;
    };

private:
    bool _error = false;
    std::string _pattern;
    std::vector<format_item*> _items;
};

class logger
{
public:
    logger();
    void log(LOG_LEVEL level, log_event* event);

    void add_appender(const std::string& name, log_appender* appender);
    void del_appender(const std::string& name);
    void clr_appender();
private:
    log_appender*  _root_appender  = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

}