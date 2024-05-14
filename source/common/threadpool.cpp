#include <assert.h>
#include <stdexcept>
#include "threadpool.h"
#include "timewheel.h"

thread_group::thread_group()
{
    memset(_threads, 0, sizeof(_threads));
    _stop = false;
    for(size_t i = 0; i < THREAD_COUNT_PER_GROUP; ++i)
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
                    printf("thread_task run throw exception!!!");
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
    for(size_t i = 0 ; i < THREAD_COUNT_PER_GROUP; ++i)
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
    if(_task_queue.size() >= TASK_QUEUE_MAX)
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
    for(size_t i = 0 ; i < THREAD_GROUP_MAX; ++i)
    {
        if(_groups[i] == nullptr)
        {
            _groups[i] = new thread_group();
        }
        else
        {
            printf("thread_group %d already start!!!", (int)i);
        }
    }
}

void threadpool::stop()
{
    for(size_t i = 0 ; i < THREAD_GROUP_MAX; ++i)
    {
        if(_groups[i] && _groups[i]->wait_for_all_done(0))
        {
            delete _groups[i];
        }
    }
    memset(_groups, 0, sizeof(_groups));
}

void threadpool::add_task(int groupidx, const std::function<void()>& task)
{
    assert(groupidx >= 0 && groupidx < THREAD_GROUP_MAX);
    _groups[groupidx]->add_task(task);
}
