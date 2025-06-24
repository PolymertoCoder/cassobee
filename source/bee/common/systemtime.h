#pragma once
#include <ctime>
#include <sys/time.h>
#include <string>
#include "macros.h"
#include "types.h"

namespace bee
{

static constexpr TIMETYPE INVALID_TIME = -1;

class systemtime
{
public:
    static TIMETYPE get_time()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<TIMETYPE>(ts.tv_sec);
    }
    static struct tm get_local_time()
    {
        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);
        return tm;
    }
    static TIMETYPE get_seconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<TIMETYPE>(ts.tv_sec);
    }
    static TIMETYPE get_millseconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<TIMETYPE>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
    }
    static TIMETYPE get_microseconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<TIMETYPE>(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
    }
    static TIMETYPE get_nanoseconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<TIMETYPE>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
    }
    static std::string format_time(TIMETYPE now = INVALID_TIME, const char* fmt = "%Y-%m-%d %H:%M:%S")
    {
        thread_local char timestr[20];
        if(now == INVALID_TIME) now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);
        ::strftime(timestr, sizeof(timestr), fmt, &tm);
        return timestr;
    }
    static TIMETYPE get_today_start(TIMETYPE curtime = INVALID_TIME)
    {
        if(curtime == INVALID_TIME) curtime = time(NULL);
        struct tm tm1;
        localtime_r(&curtime, &tm1);

        struct tm tm2;
        tm2.tm_year = tm1.tm_year;
        tm2.tm_mon  = tm1.tm_mon;
        tm2.tm_mday = tm1.tm_mday;
        return mktime(&tm2);
    }
    static TIMETYPE get_nextday_start(TIMETYPE curtime = INVALID_TIME)
    {
        TIMETYPE today_start = get_today_start(curtime);
        return today_start + ONEDAY;
    }
    static TIMETYPE get_curhour_start(TIMETYPE curtime = INVALID_TIME)
    {
        if(curtime == INVALID_TIME) curtime = time(NULL);
        struct tm tm1;
        localtime_r(&curtime, &tm1);

        struct tm tm2;
        tm2.tm_year = tm1.tm_year;
        tm2.tm_mon  = tm1.tm_mon;
        tm2.tm_mday = tm1.tm_mday;
        tm2.tm_hour = tm1.tm_hour;
        return mktime(&tm2);
    }
    static TIMETYPE get_nexthour_start(TIMETYPE curtime = INVALID_TIME)
    {
        TIMETYPE curhour_start = get_curhour_start(curtime);
        return curhour_start + ONEHOUR;
    }
};

} // namespace bee