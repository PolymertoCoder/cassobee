#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctime>
#include <cstdarg>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>

#include "util.h"

int set_nonblocking(int fd, bool nonblocking)
{
    int flags = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, nonblocking ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));
}

int rand(int min, int max)
{
    srand(time(NULL));
    return (rand() % (max - min + 1)) + min;
}

pid_t gettid()
{
    return syscall(SYS_gettid);
}

void set_process_affinity(int cpu_num)
{
    pid_t self = gettid();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_num, &mask);
    sched_setaffinity(self, sizeof(mask), &mask);
}

std::string format_string(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
}