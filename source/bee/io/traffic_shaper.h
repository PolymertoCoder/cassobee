#pragma once
#include "lock.h"
#include "systemtime.h"

namespace bee
{

class traffic_shaper
{
public:
    traffic_shaper(size_t rate_bps) 
        : _capacity(rate_bps), _tokens(rate_bps), _last_fill(systemtime::get_millseconds()) {}
    
    bool acquire(size_t bytes)
    {
        bee::spinlock::scoped l(_lock);
        auto now = systemtime::get_millseconds();
        auto elapsed = now - _last_fill;
        
        // 计算新增令牌
        size_t new_tokens = elapsed * (_capacity / 1000);
        _tokens = std::min(_capacity, _tokens + new_tokens);
        _last_fill = now;
        
        if(_tokens >= bytes)
        {
            _tokens -= bytes;
            return true;
        }
        return false;
    }

private:
    bee::spinlock _lock;
    size_t _capacity = 0; // 令牌桶容量（字节/秒）
    size_t _tokens = 0;
    TIMETYPE _last_fill = 0;
}; 

} // namespace bee