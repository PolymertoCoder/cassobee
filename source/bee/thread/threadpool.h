#pragma once
#include <cstring>
#include <atomic>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "types.h"
#include "common.h"

// TODO 线程本地任务

using TASK_QUEUE = std::deque<std::function<void()>>;

inline thread_local TASK_QUEUE _essential_task_queue;

class threadpool;

class thread_group
{
public:
    
    thread_group(size_t idx, size_t maxsize, size_t threadcnt);
    ~thread_group();
    void add_task(const std::function<void()>& task);
    bool has_task() const { return !_essential_task_queue.empty() || !_task_queue.empty(); }
    
    void notify_one();
    bool wait_for_all_done(TIMETYPE millsecond);

private:
    friend threadpool;
    const size_t _idx       = 0;
    const size_t _maxsize   = 0;
    const size_t _threadcnt = 0;

    std::atomic_bool _stop = true;
    std::atomic_int _busy = 0;
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
    void add_essential_task(const std::function<void()>& task);

    bool is_steal() { return _is_steal; }
    void try_steal_one(int current_idx, std::function<void()>& task);
    
private:
    friend thread_group;
    std::atomic_bool _stop = true;
    std::atomic_bool _is_steal = false;
    std::vector<thread_group*> _groups;
};
