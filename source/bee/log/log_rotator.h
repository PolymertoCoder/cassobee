#pragma once
#include <string>
#include "types.h"

namespace bee
{

class rotatable_log_appender;

// 负责日志的流转功能
class log_rotator
{
public:
    log_rotator(rotatable_log_appender* appender)
        : _appender(appender) {}
    virtual ~log_rotator() = default;
    virtual bool check_rotate() = 0;
    virtual std::string get_suffix() { return _suffix; }

protected:
    rotatable_log_appender* _appender = nullptr;
    TIMERID _check_rotate_timerid = -1;
    std::string _suffix; // 日志文件名后缀
};

// 根据时间流转日志
class time_log_rotator : public log_rotator
{
public:
    enum ROTATE_TYPE : uint8_t
    {
        ROTATE_TYPE_HOUR, // 按小时分割日志
        ROTATE_TYPE_DAY,  // 按自然日分割日志
    };

    time_log_rotator(rotatable_log_appender* appender, ROTATE_TYPE rotate_type);
    virtual ~time_log_rotator() override;
    virtual bool check_rotate() override;

private:
    ROTATE_TYPE _rotate_type;
    TIMETYPE    _next_rotate_time = 0; // 下一次分割日志的时间
};

// 根据日志文件大小流转
class size_log_rotator : public log_rotator
{
public:
    size_log_rotator(rotatable_log_appender* appender, size_t threshold);
    virtual ~size_log_rotator() override;
    virtual bool check_rotate() override;

private:
    size_t _threshold = 0;
};

} // namespace bee