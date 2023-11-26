#include <fcntl.h>
#include <stdlib.h>
#include <ctime>

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
