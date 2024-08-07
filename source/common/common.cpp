#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/times.h>
#include <sched.h>
#include <cstring>
#include <ctime>
#include <cstdarg>

#include "common.h"

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
    thread_local pid_t tid = syscall(SYS_gettid);
    return tid;
}

TIMETYPE get_process_elapse()
{
    static struct tms start_tms;
    const static long start_time = times(&start_tms);
    const static long sc_clk_tck = syscall(_SC_CLK_TCK);
    
    struct tms now_tms;
    long now_time = times(&now_tms);
    // TIMETYPE realtime = (now_time - start_time) / sc_clk_tck;
    // TIMETYPE usertime = (now_tms.tms_utime - start_tms.tms_utime) / sc_clk_tck;
    // TIMETYPE systime  = (now_tms.tms_stime - start_tms.tms_stime) / sc_clk_tck;
    return (now_time - start_time) / sc_clk_tck;
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
    thread_local char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}

std::string trim(const std::string_view& str, const char c)
{
    size_t begin = str.find_first_not_of(c);
    if(begin == std::string::npos) return "";

    size_t end = str.find_last_not_of(c);
    if(end == std::string::npos) return "";
    return std::string(str.substr(begin, end - begin + 1));
}

std::vector<std::string> split(const std::string_view& str, const char* delim)
{
    thread_local char data[1024];
    str.copy(data, sizeof(data));
    std::vector<std::string> result;

    char* token = strtok(data, delim);
    while(token != NULL)
    {
        result.push_back(token);
        token = strtok(NULL, delim);
    }
    return result;
}
