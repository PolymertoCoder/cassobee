#include "log_rotator.h"
#include "log_appender.h"
#include "reactor.h"
#include <filesystem>

namespace bee
{

time_log_rotator::time_log_rotator(rotatable_log_appender* appender, ROTATE_TYPE rotate_type)
    : log_rotator(appender), _rotate_type(rotate_type)
{
    check_rotate();
    _check_rotate_timerid = add_timer(10*1000, [this]()
    {
        if(check_rotate())
        {
            _appender->rotate();
        }
        return true;
    });
}

time_log_rotator::~time_log_rotator()
{
    if(_check_rotate_timerid >= 0)
    {
        del_timer(_check_rotate_timerid);
        _check_rotate_timerid = -1;
    }
}

bool time_log_rotator::check_rotate()
{
    TIMETYPE curtime = systemtime::get_time();
    if(curtime < _next_rotate_time) return false;

    switch(_rotate_type)
    {
        case ROTATE_TYPE_HOUR:
        {
            // 按小时分割日志并命名 yyyymmddhh
            if(_suffix.size())
            {
                _pre_suffix = _suffix;
            }
            else
            {
                _pre_suffix = systemtime::format_time(curtime - ONEHOUR, "%Y%m%d%H");
            }
            _suffix = systemtime::format_time(curtime, "%Y%m%d%H");
            _next_rotate_time = systemtime::get_nexthour_start(curtime);
            break;
        }
        case ROTATE_TYPE_DAY:
        {
            // 按自然日分割日志并命名 yyyymmdd
            if(_suffix.size())
            {
                _pre_suffix = _suffix;
            }
            else
            {
                _pre_suffix = systemtime::format_time(curtime - ONEDAY, "%Y%m%d");
            }
            _suffix = systemtime::format_time(curtime, "%Y%m%d");
            _next_rotate_time = systemtime::get_nextday_start(curtime);
            break;
        }
        default:
        {
            CHECK_BUG(false, printf("unknown rotate_type:%d\n", _rotate_type););
            return false;
        }
    }
    return true;
}

size_log_rotator::size_log_rotator(rotatable_log_appender* appender, size_t threshold)
    : log_rotator(appender), _threshold(threshold)
{
    check_rotate();
}

size_log_rotator::~size_log_rotator()
{
    if(_check_rotate_timerid >= 0)
    {
        del_timer(_check_rotate_timerid);
        _check_rotate_timerid = -1;
    }
}

bool size_log_rotator::check_rotate()
{
    if(auto* appender = dynamic_cast<file_appender*>(_appender))
    {
        std::string filepath = appender->get_filepath();
        if(std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath))
        {
            if(std::filesystem::file_size(filepath) >= _threshold)
            {
                TIMETYPE curtime = systemtime::get_time();
                if(_suffix.size())
                {
                    _pre_suffix = _suffix;
                }
                else
                {
                    _pre_suffix = systemtime::format_time(curtime - ONEHOUR, "%Y%m%d%H%M%S");
                }
                _suffix = systemtime::format_time(curtime, "%Y%m%d%H%M%S");
                return true;
            }
        }
    }
    return false;
}

} // namespace bee