#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <string_view>

#include "types.h"

namespace bee
{

#define GET_TIME_BEGIN() \
struct timeval begintime; \
gettimeofday(&begintime, NULL); \

#define GET_TIME_END() \
struct timeval endtime; \
gettimeofday(&endtime, NULL); \
printf("used time: %ld.\n", (endtime.tv_sec*1000 + endtime.tv_usec/1000) - (begintime.tv_sec*1000 + begintime.tv_usec/1000));

void set_signal(int signum, SIG_HANDLER handler);
int set_nonblocking(int fd, bool nonblocking = true);
pid_t gettid();
TIMETYPE get_process_elapse();
void set_process_affinity(int num);
std::vector<pid_t> find_pids_by_name(const std::string& name);
std::string format_string(const char* fmt, ...) FORMAT_PRINT_CHECK(1, 2);

// string op
std::string ltrim(const std::string& str, std::string_view delim = " \t\n\r");
std::string rtrim(const std::string& str, std::string_view delim = " \t\n\r");
std::string trim(std::string_view str, std::string_view delim = " \t\n\r");
std::vector<std::string> split(const std::string_view& str, std::string_view delim = " ");
bool startswith(const std::string_view& str, const std::string_view& prefix);
bool endswith(const std::string_view& str, const std::string_view& suffix);

} // namespace bee