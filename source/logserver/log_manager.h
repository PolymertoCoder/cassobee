#pragma once
#include <stdio.h>
#include <unordered_map>

#include "log.h"
#include "objectpool.h"
#include "common.h"
#include "log_event.h"

namespace cassobee
{

class logger
{
public:
    logger(LOG_LEVEL level, log_appender* appender);
    ~logger();
    void log(LOG_LEVEL level, log_event* event);

    log_appender* get_appender(const std::string& name);
    bool add_appender(const std::string& name, log_appender* appender);
    bool del_appender(const std::string& name);
    void clr_appender();

private:
    LOG_LEVEL _loglevel = LOG_LEVEL_DEBUG;
    log_appender* _root_appender = nullptr;
    std::unordered_map<std::string, log_appender*> _appenders;
};

class log_manager : public singleton_support<log_manager>
{
public:
    void init();

    template<typename ...params_type>
    void log(LOG_LEVEL level, params_type... params)
    {
        auto [id, evt] = _eventpool.alloc();
        if(PREDICT_FALSE(!evt))
        {
            printf("logevent pool is full or not initialized\n");
            return;
        }
        *evt = log_event(params...);
        _file_logger->log(level, evt);
        _eventpool.free(id);
    }

    template<typename ...params_type>
    void console_log(LOG_LEVEL level, params_type... params)
    {
        auto [id, evt] = _eventpool.alloc();
        if(PREDICT_FALSE(!evt))
        {
            printf("logevent pool is full or not initialized\n");
            return;
        }
        *evt = log_event(params...);
        _console_logger->log(level, evt);
        _eventpool.free(id);
    }

    logger* get_logger(std::string name);
    bool add_logger(std::string name, logger* logger);
    bool del_logger(std::string name);

    static void set_process_name(const char* name) { _process_name = name; }
    static const char* get_process_name() { return _process_name.data(); }
    
private:
    static std::string _process_name;
    logger* _file_logger = nullptr;
    logger* _console_logger = nullptr;
    std::unordered_map<std::string, logger*> _loggers;
    objectpool<log_event> _eventpool;
};

} // namespace cassobee