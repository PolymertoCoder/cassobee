#include "log_rotator.h"
#include "log_appender.h"
#include "reactor.h"
#include "timewheel.h"
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

    tm tm_val = systemtime::get_local_time();
    char suffix[16];
    switch(_rotate_type)
    {
        case ROTATE_TYPE_HOUR:
        {
            // 按小时分割日志并命名 yyyymmddhh
            std::snprintf(suffix, sizeof(suffix), "%04d%02d%02d%02d", 
                    tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday, tm_val.tm_hour);
            _suffix = suffix;
            _next_rotate_time = systemtime::get_nexthour_start(curtime);
            break;
        }
        case ROTATE_TYPE_DAY:
        {
            // 按自然日分割日志并命名 yyyymmdd
            std::snprintf(suffix, sizeof(suffix), "%04d%02d%02d",
                    tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday);
            _suffix = suffix;
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
            return std::filesystem::file_size(filepath) >= _threshold;
        }
    }
    return false;
}

} // namespace bee