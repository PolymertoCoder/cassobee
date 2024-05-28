#pragma once
#include <ctime>
#include <sys/time.h>
#include <string>
#include "types.h"

class systemtime
{
public:
    static TIMETYPE get_time()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec;
    }
    static struct tm get_local_time()
    {
        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);
        return tm;
    }
    static TIMETYPE get_millseconds()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec*1000 + tv.tv_usec/1000);
    }
    static TIMETYPE get_microseconds()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec*1000000 + tv.tv_usec);
    }
    static std::string format_time(TIMETYPE now = -1, const char* fmt = "%Y-%m-%d %H:%M:%S")
    {
        thread_local char timestr[20];
        if(now == -1) now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);
        ::strftime(timestr, sizeof(timestr), fmt, &tm);
        return timestr;
    }
    static TIMETYPE get_nextday_time()
    {
        time_t t = time(NULL);
        struct tm tm1;
        localtime_r(&t, &tm1);

        struct tm tm2;
        tm2.tm_year = tm1.tm_year;
        tm2.tm_mon  = tm1.tm_mon;
        tm2.tm_mday = tm1.tm_mday + 1;
        return mktime(&tm2);
    }
};