#pragma once
#include <cstring>
#include <atomic>
#include <deque>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "../types.h"

#define THREAD_COUNT_PER_GROUP 4
#define THREAD_GROUP_MAX 4
#define TASK_QUEUE_MAX 524288

class thread_group
{
public:
    using TASK_QUEUE = std::deque<std::function<void()>>;
    thread_group();
    ~thread_group();
    void add_task(const std::function<void()>& task);
    bool wait_for_all_done(TIMETYPE millsecond);
private:
    std::atomic_bool _stop;
    std::atomic_int _busy;
    std::mutex _queue_lock;
    std::condition_variable _cond;
    std::thread* _threads[THREAD_COUNT_PER_GROUP];
    TASK_QUEUE _task_queue;
};

class threadpool
{
public:
    static threadpool* get_instance()
    {
        static threadpool _instance;
        return &_instance;
    }
    threadpool(){ memset(_groups, 0, sizeof(_groups)); }
    void start();
    void stop();

    void add_task(int groupidx, const std::function<void()>& task);
private:
    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    threadpool& operator=(const threadpool&&) = delete;
private:
    thread_group* _groups[THREAD_GROUP_MAX];
};
