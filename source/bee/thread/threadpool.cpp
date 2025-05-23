#include <assert.h>
#include <atomic>
#include <bits/chrono.h>
#include <string>
#include <utility>

#include "threadpool.h"
#include "glog.h"
#include "config.h"
#include "common.h"

namespace bee
{

thread_group::thread_group(size_t idx, size_t maxsize, size_t threadcnt)
    : _idx(idx), _maxsize(maxsize), _threadcnt(threadcnt)
{
    _threads.resize(threadcnt);
    _stop = false;
    for(size_t i = 0; i < _threadcnt; ++i)
    {
        _threads[i] = (new std::thread([this]
        {
            auto pool = threadpool::get_instance();
            while(true)
            {
                runnable* task = nullptr;
                {
                    std::unique_lock<std::mutex> l(_queue_lock);
                    while(!has_task()) // 循环判断条件是否满足，避免cond的假唤醒，增加程序的健壮性
                    {
                        if(this->_stop.load(std::memory_order_acquire)) return;

                        if(pool->is_steal())
                        {
                            l.unlock();
                            pool->try_steal_one((int)_idx, task);
                            if(task) break;
                            l.lock();
                        }

                        this->_cond.wait(l, [this]
                        {
                            return this->_stop.load(std::memory_order_acquire) || has_task();
                        });
                    }

                    // 从任务队列里取任务
                    if(!task)
                    {
                        if(!_essential_task_queue.empty())
                        {
                            task = _essential_task_queue.front();
                            _essential_task_queue.pop_front();
                        }
                        else if(!this->_task_queue.empty())
                        {
                            task = this->_task_queue.front();
                            this->_task_queue.pop_front();
                        }
                    }
                }

                ++_busy;
                try
                {
                    task->run();
                    //local_log("thread_task run success");
                }
                catch(...)
                {
                    local_log("thread_task run throw exception!!!");
                }
                task->destroy();
                --_busy;

                {
                    std::lock_guard<std::mutex> l(this->_queue_lock);
                    if(_busy == 0 && !has_task())
                    {
                        this->_cond.notify_all(); // 这里只是为了去通知wait_for_all_done
                    }
                }
            } 
        }));
    }
}

thread_group::~thread_group()
{
    {
        std::lock_guard<std::mutex> lock(_queue_lock);
        _stop.store(true, std::memory_order_release);
    }
    _cond.notify_all();
    for(size_t i = 0 ; i < _threadcnt; ++i)
    {
        if(_threads[i]->joinable())
        {
            _threads[i]->join();
        }
        delete _threads[i];
    }
}

void thread_group::add_task(runnable* task)
{
    std::lock_guard<std::mutex> l(_queue_lock);
    if(_task_queue.size() >= _maxsize)
    {
        local_log("thread group %d task_queue is full!!!", (int)_idx);
        task->destroy();
        return;
    }
    if(_stop.load(std::memory_order_acquire))
    {
        local_log("thread group %d is stopped, add_task failed.", (int)_idx);
        task->destroy();
        return;
    }
   _task_queue.push_back(task);
   _cond.notify_one();
}

void thread_group::notify_one()
{
    _cond.notify_one();
}

bool thread_group::wait_for_all_done(TIMETYPE millsecond)
{
    std::unique_lock<std::mutex> l(_queue_lock);
    if(!has_task()){ return true; }
    if(millsecond <= 0)
    {
        _cond.wait(l, [this](){ return !has_task(); });
        return true;
    }
    else
    {
        return this->_cond.wait_for(l, std::chrono::milliseconds(millsecond), [this]{ return !has_task(); });
    }
}

void threadpool::start()
{
    auto cfg = config::get_instance();
    std::string str = cfg->get("threadpool", "groups");
    std::vector<std::pair<int, int>> groups;

    std::vector<std::string> result = split(str, "(,) ");
    assert(result.size() > 0 && result.size() % 2 == 0);
    for(size_t i = 0; i < result.size(); i += 2)
    {
        groups.emplace_back(std::stoi(result[i]), std::stoi(result[i + 1]));
    }

    _groups.resize(groups.size(), nullptr);
    for(size_t i = 0; i < groups.size() ; ++i)
    {
        const auto& [maxsize, threadcnt] = groups[i];
        if(_groups[i] == nullptr)
        {
            _groups[i] = new thread_group(i, maxsize, threadcnt);
            local_log("thread group %d run %d threads, task queue maxsize:%d.", (int)i, threadcnt, maxsize);
        }
        else
        {
            local_log("thread group %d already start.", (int)i);
        }
    }
    _is_steal = cfg->get("threadpool", "steal", false);
    if(_is_steal)
    {
        local_log("threadpool enable steal task.");
    }
    _stop.store(false, std::memory_order_release);
}

void threadpool::stop()
{
    _stop.store(true, std::memory_order_release);
    for(auto group : _groups)
    {
        if(group && group->wait_for_all_done(0))
        {
            delete group;
        }
    }
    _groups.clear();
    _groups.shrink_to_fit();
}

void threadpool::add_task(int groupidx, runnable* task)
{
    if(_stop.load(std::memory_order_acquire))
    {
        local_log("threadpool is stopped, add_task failed.");
        task->destroy();
        return;
    }
    ASSERT(groupidx >= 0 && groupidx < static_cast<int>(_groups.size()));
    _groups[groupidx]->add_task(task);
}

void threadpool::add_task(int groupidx, const std::function<void()>& task)
{
    add_task(groupidx, new functional_runnable(std::move(task)));
}

void threadpool::add_task(int groupidx, std::function<void()>&& task)
{
    add_task(groupidx, new functional_runnable(std::move(task)));
}

void threadpool::add_essential_task(runnable* task)
{
    if(_stop.load(std::memory_order_acquire))
    {
        local_log("threadpool is stopped, add_essential_task failed.");
        task->destroy();
        return;
    }
    _essential_task_queue.push_back(task);
}

void threadpool::add_essential_task(const std::function<void()>& task)
{
    add_essential_task(new functional_runnable(std::move(task)));
}

void threadpool::add_essential_task(std::function<void()>&& task)
{
    add_essential_task(new functional_runnable(std::move(task)));
}

void threadpool::try_steal_one(int current_idx, runnable*& task)
{
    int group_size = _groups.size();
    int offset = rand(0, group_size - 1);
    for(auto i = 0; i < group_size; ++i)
    {
        size_t idx = (offset + i) % group_size;
        if(idx == (size_t)current_idx) continue;

        std::unique_lock<std::mutex> l(_groups[idx]->_queue_lock, std::try_to_lock);
        if(l.owns_lock() && _groups[idx]->has_task())
        {
            task = _groups[idx]->_task_queue.front();
            _groups[idx]->_task_queue.pop_front();
            if(_groups[idx]->has_task())
            {
                _groups[idx]->notify_one(); // 继续唤醒等待中的线程来窃取任务
            }
            local_log("thread group %d steal group %d task success.", current_idx, (int)_groups[idx]->_idx);
            return;
        }
    }
}

} // namespace bee