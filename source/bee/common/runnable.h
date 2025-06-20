#pragma once
#include <functional>
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
public:
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

} // namespace bee