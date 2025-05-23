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

namespace bee
{

class runnable
{
public:
    virtual ~runnable() = default;
    virtual void run() = 0;
    virtual void destroy() { delete this; }
};

class static_runnable : public runnable
{
    virtual void destroy() override {}
};

class functional_runnable : public runnable
{
public:
    functional_runnable(const std::function<void()>& func) : _func(func) {}
    functional_runnable(std::function<void()>&& func) : _func(std::move(func)) {}
    FORCE_INLINE virtual void run() override { if(_func) _func(); }
    std::function<void()> _func;
};

using TASK_QUEUE = std::deque<runnable*>;

inline thread_local TASK_QUEUE _essential_task_queue;

class threadpool;

class thread_group
{
public:
    
    thread_group(size_t idx, size_t maxsize, size_t threadcnt);
    ~thread_group();
    void add_task(runnable* task);
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

    void add_task(int groupidx, runnable* task);
    void add_task(int groupidx, const std::function<void()>& task);
    void add_task(int groupidx, std::function<void()>&& task);

    void add_essential_task(runnable* task);
    void add_essential_task(const std::function<void()>& task);
    void add_essential_task(std::function<void()>&& task);

    bool is_steal() { return _is_steal; }
    void try_steal_one(int current_idx, runnable*& task);
    
private:
    friend thread_group;
    std::atomic_bool _stop = true;
    std::atomic_bool _is_steal = false;
    std::vector<thread_group*> _groups;
};

} // namespace bee