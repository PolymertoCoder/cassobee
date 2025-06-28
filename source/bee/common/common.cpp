#include "common.h"
#include <cstdarg>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/times.h>
#include <sched.h>

namespace bee
{

void set_signal(int signum, SIG_HANDLER handler)
{
    struct sigaction act{};
    act.sa_handler = handler; // 设置信号处理函数
    sigfillset(&act.sa_mask); // 初始化信号屏蔽集
    act.sa_flags |= SA_RESTART; // 由此信号中断的系统调用自动重启动

    if(sigaction(signum, &act, NULL) == -1)
    {
        printf("capture %d signal, but to deal with failure", signum);
    }
}

int set_nonblocking(int fd, bool nonblocking)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1) return -1;
    return fcntl(fd, F_SETFL, nonblocking ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));
}

pid_t gettid()
{
    thread_local pid_t tid = syscall(SYS_gettid);
    return tid;
}

// 获取进程运行时间（秒）
TIMETYPE get_process_elapse()
{
    static struct tms start_tms{};
    const static long start_time = times(&start_tms);
    const static long sc_clk_tck = sysconf(_SC_CLK_TCK);

    struct tms now_tms{};
    long now_time = times(&now_tms);
    // TIMETYPE realtime = (now_time - start_time) / sc_clk_tck;
    // TIMETYPE usertime = (now_tms.tms_utime - start_tms.tms_utime) / sc_clk_tck;
    // TIMETYPE systime  = (now_tms.tms_stime - start_tms.tms_stime) / sc_clk_tck;
    return (now_time - start_time) / sc_clk_tck;
}

// 绑定当前线程到指定 CPU
void set_process_affinity(int cpu_num)
{
    pid_t self = gettid();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_num, &mask);
    sched_setaffinity(self, sizeof(mask), &mask);
}

// 查找所有包含指定名称的进程 PID
std::vector<pid_t> find_pids_by_name(const std::string& name)
{
    std::vector<pid_t> pids;

    for(const auto& entry : std::filesystem::directory_iterator("/proc"))
    {
        if (!entry.is_directory()) continue;

        const std::string pid_str = entry.path().filename();
        if (!std::all_of(pid_str.begin(), pid_str.end(), ::isdigit)) continue;

        std::ifstream cmdline_file(entry.path() / "cmdline");
        std::string cmdline;
        std::getline(cmdline_file, cmdline);

        if(cmdline.find(name) != std::string::npos)
        {
            pids.push_back(std::stoi(pid_str));
        }
    }

    return pids;
}

// 格式化字符串（使用 C++20 std::format）
std::string format_string(const char* fmt, ...)
{
    thread_local char buf[1024*4];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}

// 左裁剪
std::string ltrim(const std::string& str, std::string_view delim)
{
    auto begin = str.find_first_not_of(delim);
    return begin != std::string::npos ? str.substr(begin) : "";
}

// 右裁剪
std::string rtrim(const std::string& str, std::string_view delim)
{
    auto end = str.find_last_not_of(delim);
    return end != std::string::npos ? str.substr(0, end + 1) : "";
}

// 全裁剪
std::string trim(std::string_view str, std::string_view delim)
{
    size_t begin = str.find_first_not_of(delim);
    if(begin == std::string::npos) return "";

    size_t end = str.find_last_not_of(delim);
    return std::string(str.substr(begin, end - begin + 1));
}

// 字符串分割
std::vector<std::string> split(const std::string_view& str, std::string_view delim)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end;

    while((end = str.find_first_of(delim, start)) != std::string_view::npos)
    {
        if(end > start)
        {
            result.emplace_back(str.substr(start, end - start));
        }
        start = end + 1;
    }

    if(start < str.size())
    {
        result.emplace_back(str.substr(start));
    }

    return result;
}

// 前缀判断
bool startswith(const std::string_view& str, const std::string_view& prefix)
{
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

// 后缀判断
bool endswith(const std::string_view& str, const std::string_view& suffix)
{
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

} // namespace bee
