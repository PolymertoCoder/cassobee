#pragma once 
#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "types.h"

namespace bee
{

class async_threadpool
{
public:
    async_threadpool( size_t threads_count ) : _threads_count(threads_count), _stop(true){}
    ~async_threadpool(){}

    async_threadpool( const async_threadpool& ) = delete;
    async_threadpool( async_threadpool&& ) = delete;
    async_threadpool& operator=( const async_threadpool& ) = delete;
    async_threadpool& operator=( const async_threadpool&& ) = delete;

public:
    void init()
    {
        _stop = false;
        for( size_t i = 0; i < _threads_count; ++i )
        {
            workers.emplace_back( new std::thread( [this]
            {
                for(;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->_queue_lock);
                        this->_cond.wait(lock, [this]{ return this->_stop || !this->_tasks.empty(); });

                        if(this->_stop && this->_tasks.empty()){ return; } // 再次判断条件是否满足，避免cond的假唤醒，增加程序的健壮性
                        // 从任务队列里取任务
                        task = std::move(this->_tasks.front());
                        this->_tasks.pop();
                    }
                    ++_busy;
                    try{ task(); }
                    catch(...) {
                        std::cout << "task run throw exception!!!" << std::endl;
                    }
                    --_busy;

                    std::unique_lock<std::mutex> lock(this->_queue_lock);
                    if( _busy == 0 && _tasks.empty() )
                    {
                        this->_cond.notify_all(); // 这里只是为了去通知wait_for_all_done
                    }
                } 
            }) );
        }
    }

    // 添加新的任务到任务队列
    template<class F, class... Args>
    auto add_task(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        // 获取函数的返回值类型
        using return_type = typename std::result_of<F(Args...)>::type;

        // 创建一个指向任务的智能指针
        auto task = std::make_shared< std::packaged_task<return_type()> >( std::bind(std::forward<F>(f), std::forward<Args>(args)...) );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_queue_lock);
            if(_stop){ throw std::runtime_error("add_task on stopped threadpool"); }

            _tasks.emplace([task](){ (*task)(); }); // 把任务加入队列
        }
        _cond.notify_one(); // 通知条件变量，唤醒一个线程
        return res;
    }

    void stop()
    {
        {
            std::unique_lock<std::mutex> lock(_queue_lock);
            _stop = true;
        }
        _cond.notify_all();
        for( size_t i = 0 ; i < _threads_count; ++i )
        {
            if(workers[i]->joinable())
            {
                workers[i]->join();
            }
            delete workers[i];
        }

        std::unique_lock<std::mutex> lock(_queue_lock);
        workers.clear();
    }

    bool wait_for_all_done( TIMETYPE millsecond )
    {
        std::unique_lock<std::mutex> lock(this->_queue_lock);

        if( _tasks.empty() ){ return true; }
        if( millsecond <= 0 )
        {
            this->_cond.wait(lock, [this](){ return this->_tasks.empty(); });
            return true;
        }
        else
        {
            return this->_cond.wait_for(lock, std::chrono::milliseconds(millsecond), [this]{ return this->_tasks.empty(); });
        }
    }

private:
    size_t _threads_count;
    std::vector<std::thread*> workers;
    std::queue<std::function<void()>> _tasks;

    std::atomic_bool _stop;
    std::atomic_int _busy;
    std::mutex _queue_lock; 
    std::condition_variable _cond;
};

} // namespace bee