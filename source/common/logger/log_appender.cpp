#include <sys/stat.h>

#include "log_appender.h"

namespace cassobee
{

time_rotater::time_rotater(ROTATE_TYPE rotate_type) : _rotate_type(rotate_type) {}

bool time_rotater::is_rotate()
{
    tm* tm_val = systemtime::get_local_time();
    if(_rotate_type == ROTATE_TYPE_HOUR)
    {
        // 按小时分割日志并命名 yyyymmddhh
        tm* tm_val = systemtime::get_local_time();
        return static_cast<uint64_t>(tm_val->tm_year + 1900) * 1000000 + static_cast<uint64_t>(tm_val->tm_mon + 1) * 10000 +
            static_cast<uint64_t>(tm_val->tm_mday) * 100 + static_cast<uint64_t>(tm_val->tm_hour);
    }
    else if(_rotate_type == ROTATE_TYPE_DAY)
    {
        // 按自然日分割日志并命名 yyyymmdd
        return static_cast<uint64_t>(tm_val->tm_year + 1900) * 10000 + static_cast<uint64_t>(tm_val->tm_mon + 1) * 100 +
               static_cast<uint64_t>(tm_val->tm_mday);
    }
    return false;
}

file_appender::file_appender(std::string filedir, std::string filename)
    : _filedir(filedir), _filename(filename)
{
    if(_filedir.empty())
    {
        _filedir = ".";
    }
    _filepath = _filedir + '/' + filename;
}

file_appender::~file_appender()
{
    if(_filestream.is_open())
    {
        _filestream.close();
    }
}

bool file_appender::init()
{
    int ret = mkdir(_filedir.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(ret != 0 && errno != EEXIST)
    {
        printf("mkdir failed, dir:%s err:%s", _filedir.data(), strerror(errno));
        return false;
    }
    _filestream.open(_filepath, std::fstream::out | std::fstream::app);
    return true;
}

void file_appender::log(LOG_LEVEL level, log_event* event)
{
    _formatter->format(level, event);
}

async_appender::async_appender(std::string logdir, std::string filename)
    : file_appender(logdir, filename)
{
}

void async_appender::log(LOG_LEVEL level, log_event* event)
{

}

}