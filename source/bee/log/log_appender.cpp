#include <filesystem>
#include <errno.h>
#include <stdint.h>
#include <cstdio>
#include <string>

#include "log_appender.h"
#include "config.h"
#include "log_formatter.h"
#include "reactor.h"
#include "systemtime.h"
#include "common.h"
#include "timewheel.h"

namespace bee
{

log_appender::log_appender()
{
    std::string format_pattern = config::get_instance()->get("log", "pattern");
    _formatter = new log_formatter(format_pattern);
}

void console_appender::log(LOG_LEVEL level, const log_event& event)
{
    std::string msg = _formatter->format(level, event);
#ifdef _REENTRANT
    std::unique_lock<std::mutex> lock(_locker);
#endif
    printf("%s", msg.data());
    fflush(stdout);
}

///////////////////////////////// log_rotater begin ///////////////////////////////

// 负责日志的流转功能
class rotatable_log_appender::log_rotater
{
public:
    log_rotater(rotatable_log_appender* appender)
        : _appender(appender) {}
    virtual ~log_rotater() = default;
    virtual std::string get_suffix() { return _suffix; }

protected:
    rotatable_log_appender* _appender = nullptr;
    int  _check_rotate_timerid = -1;
    std::string _suffix; // 日志文件名后缀
};

// 根据时间流转日志
class rotatable_log_appender::time_log_rotater : public log_rotater
{
public:
    enum ROTATE_TYPE : uint8_t
    {
        ROTATE_TYPE_HOUR, // 按小时分割日志
        ROTATE_TYPE_DAY,  // 按自然日分割日志
    };

    time_log_rotater(rotatable_log_appender* appender, ROTATE_TYPE rotate_type)
        : log_rotater(appender), _rotate_type(rotate_type)
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

    virtual ~time_log_rotater()
    {
        if(_check_rotate_timerid >= 0)
        {
            bool ret = timewheel::get_instance()->del_timer(_check_rotate_timerid);
            CHECK_BUG(ret, printf("remove invalid check rotate timer %d ???", _check_rotate_timerid));
            _check_rotate_timerid = -1;
        }
    }

    bool check_rotate()
    {
        TIMETYPE curtime = systemtime::get_time();
        return curtime >= _next_rotate_time && update(curtime);
    }

    bool update(TIMETYPE curtime)
    {
        tm tm_val = systemtime::get_local_time();
        switch(_rotate_type)
        {
            case ROTATE_TYPE_HOUR:
            {
                // 按小时分割日志并命名 yyyymmddhh
                uint64_t suffix = static_cast<uint64_t>(tm_val.tm_year + 1900) * 1000000 +
                                  static_cast<uint64_t>(tm_val.tm_mon + 1) * 10000 +
                                  static_cast<uint64_t>(tm_val.tm_mday) * 100 +
                                  static_cast<uint64_t>(tm_val.tm_hour);
                _suffix = std::to_string(suffix);
                _next_rotate_time = systemtime::get_nexthour_start(curtime);
                break;
            }
            case ROTATE_TYPE_DAY:
            {
                // 按自然日分割日志并命名 yyyymmdd
                uint64_t suffix = static_cast<uint64_t>(tm_val.tm_year + 1900) * 10000 +
                                  static_cast<uint64_t>(tm_val.tm_mon + 1) * 100 +
                                  static_cast<uint64_t>(tm_val.tm_mday);
                _suffix = std::to_string(suffix);
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

private:
    ROTATE_TYPE _rotate_type;
    TIMETYPE    _next_rotate_time = 0; // 下一次分割日志的时间
};

///////////////////////////////// log_rotater end ///////////////////////////////

rotatable_log_appender::rotatable_log_appender(log_rotater* rotater)
    : _rotater(rotater)
{
}

rotatable_log_appender::~rotatable_log_appender()
{
    if(_rotater)
    {
        delete _rotater;
        _rotater = nullptr;
    }
}

std::string rotatable_log_appender::get_suffix()
{
    return _rotater ? _rotater->get_suffix() : "";
}

file_appender::file_appender(std::string filedir, std::string filename)
    : rotatable_log_appender(new time_log_rotater(this, time_log_rotater::ROTATE_TYPE_DAY))
    , _filedir(filedir)
    , _filename(filename)
{
    if(_filedir.empty())
    {
        _filedir = ".";
    }

    if(!std::filesystem::create_directories(_filedir.data()) && errno != EEXIST)
    {
        printf("create directory failed, dir:%s err:%s\n", _filedir.data(), strerror(errno));
        return;
    }

    reopen();
    _running = true;
}

file_appender::~file_appender()
{
    if(_filestream.is_open())
    {
        _filestream.close();
    }
}

bool file_appender::rotate()
{
#ifdef _REENTRANT
    std::unique_lock<std::mutex> lock(_locker);
#endif
    return reopen();
}

void file_appender::log(LOG_LEVEL level, const log_event& event)
{
    if(!_running) return;
    std::string msg = _formatter->format(level, event);

#ifdef _REENTRANT
    std::unique_lock<std::mutex> lock(_locker);
#endif
    _filestream << msg;
    _filestream.flush();
}

bool file_appender::reopen() // no lock
{
    _filepath = _filedir + format_string("/%s.%s.log", _filename.data(), get_suffix().data());
    if(_filestream.is_open())
    {
        _filestream.close();
    }
    _filestream.open(_filepath, std::fstream::out | std::fstream::app);
    printf("open filestream %s\n", _filepath.data());
    return true;
}

async_appender::async_appender(std::string logdir, std::string filename)
    : file_appender(logdir, filename)
{
    auto cfg = config::get_instance();
    _timeout   = cfg->get<int>("log", "interval");
    _threshold = cfg->get<int>("log", "threshold");
    start();
}

async_appender::~async_appender()
{
    stop();
}

void async_appender::log(LOG_LEVEL level, const log_event& event)
{
    if(!_running) return;
    std::string msg = _formatter->format(level, event);

    std::unique_lock<std::mutex> lock(_locker);
    size_t write_len = _buf.write(msg.data(), msg.size());
    if(PREDICT_FALSE(write_len != msg.size()))
    {
        printf("log lost!!! message size:%zu real write size:%zu.\n", msg.size(), write_len);
    }

    size_t buf_len = _buf.size();
    if(buf_len - write_len < _threshold && buf_len >= _threshold)
    {
        _cond.notify_one();
    }
}

void async_appender::start()
{
    _running = true;
    _thread = new std::thread([this]()
    {
        while(_running)
        {
            std::unique_lock<std::mutex> lock(_locker);
            while(_buf.empty())
            {
                _cond.wait_for(lock, std::chrono::milliseconds(_timeout),
                    [this](){ return _buf.size() >= _threshold || !_running; });
            }

            if(size_t length = _buf.read(_filestream, _buf.size()); length > 0)
            {
                _filestream.flush();
                //printf("async_appender write log %zu...\n", length);
            }
        }
    });
}

void async_appender::stop()
{
    if(!_running) return;
    _running = false;
    _cond.notify_one();
    if(_thread->joinable())
    {
        _thread->join();
    }
    delete _thread;
    _thread = nullptr;
}

}