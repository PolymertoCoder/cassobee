#pragma once
#include <unistd.h>
#include <stdio.h>
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
   T* get_instance()
   {
       static T _instance;
       return &_instance;
   }
};

int set_nonblocking(int fd, bool nonblocking);

int rand(int min, int max);
