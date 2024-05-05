#pragma once
#include <time.h>
#include "types.h"

class systemtime
{
public:
    static TIMETYPE get_time()
    {
        time_t tm;
        time(&tm);
        return tm;
    }
    static tm* get_local_time()
    {
        time_t t;
        time(&t);
        return localtime(&t);
    }
};