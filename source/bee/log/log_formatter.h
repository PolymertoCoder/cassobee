#pragma once
#include <vector>
#include <iosfwd>
#include <string>

#include "log.h"

namespace bee
{
class log_event;

// 日志格式化器
class log_formatter
{
public:
    log_formatter(const std::string pattern);
    std::string format(LOG_LEVEL level, const log_event& event);
    class format_item
    {
    public:
        virtual void format(std::ostream& os, LOG_LEVEL level, const log_event& event) = 0;
    };

private:
    bool _error = false;
    std::string _pattern;
    std::vector<format_item*> _items;
};

}