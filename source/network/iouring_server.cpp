#include <liburing.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

enum
{
    ACCEPT,
    READ,
    WRITE,
}

struct connect_info
{
    int connectfd;
    int type;
}

void 
