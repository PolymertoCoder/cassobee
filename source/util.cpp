#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctime>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>

#include "util.h"

int set_nonblocking(int fd, bool nonblocking)
{
    auto flags = fcntl(fd, F_GETFL);
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
