#include <assert.h>
#include <stdexcept>
#include "threadpool.h"
#include "config.h"
#include "log.h"

thread_group::thread_group(size_t maxsize, size_t threadcnt)
    : _maxsize(maxsize), _threadcnt(threadcnt)
{
    _threads.resize(threadcnt);
    _stop = false;
    for(size_t i = 0; i < _threadcnt; ++i)
    {
        _threads[i] = (new std::thread([this]
        {
            while(true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> l(_queue_lock);
                    while(this->_task_queue.empty()) // 循环判断条件是否满足，避免cond的假唤醒，增加程序的健壮性
                    {
                        if(this->_stop) return;
                        this->_cond.wait(l, [this]{ return this->_stop || !this->_task_queue.empty(); });
                    }

                    // 从任务队列里取任务
                    task.swap(this->_task_queue.front());
                    this->_task_queue.pop_front();
                }

                ++_busy;
                try
                {
                    task();
                }
                catch(...)
                {
                    --_busy;
                    local_log("thread_task run throw exception!!!");
                }
                --_busy;

                {
                    std::lock_guard<std::mutex> l(this->_queue_lock);
                    if(_busy == 0 && _task_queue.empty())
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
        _stop = true;
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

void thread_group::add_task(const std::function<void()>& task)
{
    std::lock_guard<std::mutex> l(this->_queue_lock);
    if(_task_queue.size() >= _maxsize)
    {
        throw std::runtime_error("task_queue is full!!!");
    }
    if(_stop){ throw std::runtime_error("add task to a stopped thread group."); }
   _task_queue.push_back(task);
   _cond.notify_one();
}

bool thread_group::wait_for_all_done(TIMETYPE millsecond)
{
    std::unique_lock<std::mutex> l(this->_queue_lock);

    if(_task_queue.empty()){ return true; }
    if(millsecond <= 0)
    {
        _cond.wait(l, [this](){ return this->_task_queue.empty(); });
        return true;
    }
    else
    {
        return this->_cond.wait_for(l, std::chrono::milliseconds(millsecond), [this]{ return this->_task_queue.empty(); });
    }
}

void threadpool::start()
{
    std::string str = config::get_instance()->get("threadpool", "groups");
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
            _groups[i] = new thread_group(maxsize, threadcnt);
            local_log("thread group %d run %d threads, task queue maxsize:%d.", (int)i, threadcnt, maxsize);
        }
        else
        {
            local_log("thread group %d already start.", (int)i);
        }
    }
}

void threadpool::stop()
{
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

void threadpool::add_task(int groupidx, const std::function<void()>& task)
{
    assert(groupidx >= 0 && groupidx < (int)_groups.size());
    _groups[groupidx]->add_task(task);
}
