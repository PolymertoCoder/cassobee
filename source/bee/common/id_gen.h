#pragma once
#include <type_traits>
#include "lock.h"

namespace bee
{

template<typename id_type, typename lock_type>
requires std::is_base_of_v<bee::lock_support<lock_type>, lock_type>
class id_generator
{
public:
    static constexpr id_type INVALID_ID = id_type(); // 默认值作为无效值使用

    virtual ~id_generator() = default;
    virtual id_type gen() = 0; // 生成一个新的ID
    virtual void reset() = 0; // 重置ID生成器
};

template<typename id_type, typename lock_type>
class sequential_id_generator : public id_generator<id_type, lock_type>
{
public:
    using base = id_generator<id_type, lock_type>;

    virtual id_type gen() override
    {
        typename lock_type::scoped l(_locker);
        return ++_nextid;
    }

    virtual void reset() override
    {
        typename lock_type::scoped l(_locker);
        _nextid = base::INVALID_ID;
    }

protected:
    lock_type _locker;
    id_type _nextid = base::INVALID_ID;
};

} // namespace bee