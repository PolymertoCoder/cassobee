#include <filesystem>
#include <stdint.h>
#include <cstdio>
#include <string>
#include <system_error>

#include "log_appender.h"
#include "config.h"
#include "log_formatter.h"
#include "log_rotator.h"
#include "common.h"

namespace bee
{

log_appender::log_appender()
{
    std::string format_pattern = config::get_instance()->get("log", "pattern");
    _formatter = new log_formatter(format_pattern);
}

void console_appender::log(const std::string& content)
{
    std::fwrite(content.data(), 1, content.size(), stdout);
    std::fflush(stdout);
}

void console_appender::log(LOG_LEVEL level, const log_event& event)
{
    std::string msg = _formatter->format(level, event);

    std::unique_lock<bee::mutex> lock(_locker);
    std::fwrite(msg.data(), 1, msg.size(), stdout);
    std::fflush(stdout);
}

rotatable_log_appender::rotatable_log_appender(log_rotator* rotator)
    : _rotator(rotator)
{
}

rotatable_log_appender::~rotatable_log_appender()
{
    if(_rotator)
    {
        delete _rotator;
        _rotator = nullptr;
    }
}

std::string rotatable_log_appender::get_pre_suffix()
{
    return _rotator ? _rotator->get_pre_suffix() : "";
}

std::string rotatable_log_appender::get_suffix()
{
    return _rotator ? _rotator->get_suffix() : "";
}

file_appender::file_appender(std::string filedir, std::string filename)
    : rotatable_log_appender(new time_log_rotator(this, time_log_rotator::ROTATE_TYPE_DAY))
    , _filedir(filedir)
    , _filename(filename)
{
    if(_filedir.empty())
    {
        _filedir = ".";
    }

    if(std::error_code ec; !std::filesystem::create_directories(_filedir.data(), ec) && ec)
    {
        printf("create directory failed, dir:%s err:%s\n", _filedir.data(), ec.message().data());
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
}

bool file_appender::rotate()
{
    std::unique_lock<bee::mutex> lock(_locker);
    return reopen();
}

void file_appender::log(const std::string& content)
{
    std::unique_lock<bee::mutex> lock(_locker);
    _filestream << content;
    _filestream.flush();
}

void file_appender::log(LOG_LEVEL level, const log_event& event)
{
    std::string msg = _formatter->format(level, event);

    std::unique_lock<bee::mutex> lock(_locker);
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

influxlog_appender::influxlog_appender(std::string logdir, std::string filename)
    : file_appender(logdir, filename) {}

bool influxlog_appender::reopen()
{
    _filepath = _filedir + format_string("/%s.log", _filename.data());
    if(_filestream.is_open())
    {
        _filestream.close();
        std::filesystem::rename(_filepath, format_string("/%s.%s.log", _filename.data(), get_pre_suffix().data()));
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

void async_appender::log(const std::string& content)
{
    std::unique_lock<bee::mutex> lock(_locker);
    size_t write_len = _buf.write(content.data(), content.size());
    if(PREDICT_FALSE(write_len != content.size()))
    {
        printf("log lost!!! content size:%zu real write size:%zu.\n", content.size(), write_len);
    }

    size_t buf_len = _buf.size();
    if(buf_len - write_len < _threshold && buf_len >= _threshold)
    {
        _cond.notify_one();
    }
}

void async_appender::log(LOG_LEVEL level, const log_event& event)
{
#ifdef _REENTRANT
    if(_running.load(std::memory_order_acquire) == false) return;
#else
    if(!_running) return;
#endif
    std::string msg = _formatter->format(level, event);

    std::unique_lock<bee::mutex> lock(_locker);
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
#ifdef _REENTRANT
    _running.store(true);
#else
    _running = true;
#endif
    _thread = new std::thread([this]()
    {
        while(true)
        {
        #ifdef _REENTRANT
            if(!_running.load(std::memory_order_acquire)) break;
        #else
            if(!_running) break;
        #endif
            std::unique_lock<bee::mutex> lock(_locker);
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
#ifdef _REENTRANT
    if(!_running.exchange(false, std::memory_order_release)) return;
#else
    if(!_running) return;
    _running = false;
#endif
    _cond.notify_one();
    if(_thread->joinable())
    {
        _thread->join();
    }
    delete _thread;
    _thread = nullptr;
}

}