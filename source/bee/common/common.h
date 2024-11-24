#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <string_view>

#include "lock.h"
#include "types.h"

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

protected:
    singleton_support() = default;
    ~singleton_support() = default;
    singleton_support(const singleton_support&) = delete;
    singleton_support(const singleton_support&&) = delete;
    singleton_support& operator=(const singleton_support&) = delete;
    singleton_support& operator=(const singleton_support&&) = delete;
};

template<typename lock_type>
struct light_object_base
{
    void lock()   { _locker.lock();   }
    void unlock() { _locker.unlock(); }
    
    lock_type _locker;
};

template<typename id_type, typename lock_type>
requires std::is_base_of_v<cassobee::lock_support<lock_type>, lock_type>
struct sequential_id_generator
{
    static constexpr id_type INVALID_ID = id_type(); // 默认值作为无效值使用
    id_type gen()
    {
        typename lock_type::scoped l(_locker);
        static id_type maxid = INVALID_ID;
        return ++maxid;
    }
    lock_type _locker;
};

void set_signal(int signum, SIG_HANDLER handler);
int set_nonblocking(int fd, bool nonblocking = true);
pid_t gettid();
TIMETYPE get_process_elapse();
int rand(int min, int max);
void set_process_affinity(int num);
std::string format_string(const char* fmt, ...) FORMAT_PRINT_CHECK(1, 2);

// 去除字符串左侧的空白字符
std::string ltrim(const std::string& str, const char c);
// 去除字符串右侧的空白字符
std::string rtrim(const std::string& str, const char c);
// 去掉字符串首尾的字符
std::string trim(const std::string_view& str, const char c = ' ');
// 按delim分割字符串
std::vector<std::string> split(const std::string_view& str, const char* delim = " ");
// 字符串是否以prefix开头
bool startswith(const std::string_view& str, const std::string_view& prefix);
// 字符串是否以suffix结尾
bool endswith(const std::string_view& str, const std::string_view& suffix);
