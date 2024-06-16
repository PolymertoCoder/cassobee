#pragma once
#include <cstring>
#include <atomic>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "types.h"
#include "common.h"

// TODO 线程本地任务

class thread_group
{
public:
    using TASK_QUEUE = std::deque<std::function<void()>>;
    thread_group(size_t maxsize, size_t threadcnt);
    ~thread_group();
    void add_task(const std::function<void()>& task);
    bool wait_for_all_done(TIMETYPE millsecond);

private:
    const size_t _maxsize   = 0;
    const size_t _threadcnt = 0;

    std::atomic_bool _stop;
    std::atomic_int _busy;
    std::mutex _queue_lock;
    std::condition_variable _cond;
    std::vector<std::thread*> _threads;
    TASK_QUEUE _task_queue;
};

class threadpool : public singleton_support<threadpool>
{
public:
    threadpool() = default;
    void start();
    void stop();

    void add_task(int groupidx, const std::function<void()>& task);

private:
    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    threadpool& operator=(const threadpool&&) = delete;
    
private:
    std::vector<thread_group*> _groups;
};
