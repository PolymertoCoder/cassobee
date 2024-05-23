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
    static tm* get_local_time()
    {
        time_t t = time(NULL);
        return localtime(&t);
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
        struct tm* local_time = localtime(&now);
        ::strftime(timestr, sizeof(timestr), fmt, local_time);
        return timestr;
    }
};