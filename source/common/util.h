#pragma once
#include "types.h"
#include <string>
#include <sys/time.h>

#define GET_TIME_BEGIN() \
struct timeval begintime; \
gettimeofday(&begintime, NULL); \

#define GET_TIME_END() \
struct timeval endtime; \
gettimeofday(&endtime, NULL); \
printf("used time: %ld.\n", (endtime.tv_sec*1000 + endtime.tv_usec/1000) - (begintime.tv_sec*1000 + begintime.tv_usec/1000));

template<typename T>
class singleton_support
{
public:
   static T* get_instance()
   {
       static T _instance;
       return &_instance;
   }
};

template<typename lock_type>
struct light_object_base
{
    void lock()   { _locker.lock();   }
    void unlock() { _locker.unlock(); }
    
    lock_type _locker;
};

int set_nonblocking(int fd, bool nonblocking);
pid_t gettid();
TIMETYPE get_process_elapse();
int rand(int min, int max);
void set_process_affinity(int num);
std::string format_string(const char* fmt, ...);

// 去掉字符串首尾的空格
std::string trim(const std::string_view& str);
