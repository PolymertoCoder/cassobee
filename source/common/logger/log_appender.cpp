#include <string>
#include <sys/stat.h>

#include "log_appender.h"
#include "log_formatter.h"
#include "macros.h"
#include "systemtime.h"

namespace cassobee
{

log_appender::log_appender()
{
    std::string format_pattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n";
    _formatter = new log_formatter(format_pattern);
}

time_rotater::time_rotater(ROTATE_TYPE rotate_type)
    : _rotate_type(rotate_type)
{
    is_rotate();
}

bool time_rotater::update(TIMETYPE curtime)
{
    tm tm_val = systemtime::get_local_time();
    if(_rotate_type == ROTATE_TYPE_HOUR)
    {
        // 按小时分割日志并命名 yyyymmddhh
        uint64_t suffix = static_cast<uint64_t>(tm_val.tm_year + 1900) * 1000000 +
                          static_cast<uint64_t>(tm_val.tm_mon + 1) * 10000 +
                          static_cast<uint64_t>(tm_val.tm_mday) * 100 +
                          static_cast<uint64_t>(tm_val.tm_hour);
        _suffix = std::to_string(suffix);
        _next_rotate_time = systemtime::get_nextday_start(curtime);
        return true;
    }
    else if(_rotate_type == ROTATE_TYPE_DAY)
    {
        // 按自然日分割日志并命名 yyyymmdd
        uint64_t suffix = static_cast<uint64_t>(tm_val.tm_year + 1900) * 10000 +
                          static_cast<uint64_t>(tm_val.tm_mon + 1) * 100 +
                          static_cast<uint64_t>(tm_val.tm_mday);
        _suffix = std::to_string(suffix);
        _next_rotate_time = systemtime::get_nexthour_start(curtime);
        return true;
    }
    CHECK_BUG(false, printf("unknown rotate_type:%d\n", _rotate_type););
    return false;
}

bool time_rotater::is_rotate()
{
    TIMETYPE curtime = systemtime::get_time();
    if(curtime < _next_rotate_time) return false;
    return update(curtime);
}

file_appender::file_appender(std::string filedir, std::string filename)
    : _filedir(filedir), _filename(filename)
{
    _rotater = new time_rotater(time_rotater::ROTATE_TYPE_DAY);
    if(_filedir.empty())
    {
        _filedir = ".";
    }
    _filepath = _filedir + format_string("/%s.%s.log", _filename.data(), _rotater->get_suffix());
    int ret = mkdir(_filedir.data(), 0777/*S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH*/);
    if(ret != 0 && errno != EEXIST)
    {
        printf("mkdir failed, dir:%s err:%s\n", _filedir.data(), strerror(errno));
        return;
    }
    reopen();
}

file_appender::~file_appender()
{
    if(_filestream.is_open())
    {
        _filestream.close();
    }
    if(_rotater)
    {
        delete _rotater;
        _rotater = nullptr;
    }
}

bool file_appender::reopen()
{
    if(_filestream.is_open())
    {
        _filestream.close();
    }
    _filestream.open(_filepath, std::fstream::out | std::fstream::app);
    printf("open filestream %s\n", _filepath.data());
    return true;
}

void file_appender::log(LOG_LEVEL level, log_event* event)
{
    std::string msg = _formatter->format(level, event);
    if(_rotater->is_rotate())
    {
        _filepath = _filedir + format_string("/%s.%s.log", _filename.data(), _rotater->get_suffix());
        reopen();
    }
    _filestream << msg;
    _filestream.flush();
}

async_appender::async_appender(std::string logdir, std::string filename)
    : file_appender(logdir, filename)
{
}

void async_appender::log(LOG_LEVEL level, log_event* event)
{

}

}