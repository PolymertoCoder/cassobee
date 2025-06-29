#pragma once
#include <assert.h>
#include <stddef.h>
#include <cstdint>
#include <functional>
#include <utility>

#include "lock.h"
#include "types.h"
#include "objectpool.h"

namespace bee
{

#define NEAR_SLOTS 4096

using callback = std::function<bool(void*)>;

enum TIMER_OBJECT_STATE
{
    TIMER_STATE_NONE,
    TIMER_STATE_ACTIVE,
    TIMER_STATE_ADD,
    TIMER_STATE_DEL,
    TIMER_STATE_MOD,
};

struct timer_node : public light_object_base<bee::spinlock>
{
    void assign()
    {
        _id = 0;
        _timeout = 0;
        _nexttime = 0;
        _handler = {};
        _param = nullptr;
        _repeats = 0;
        _state = TIMER_STATE_NONE;
        _prev = nullptr;
        _next = nullptr;
    }

    TIMERID  _id = 0;
    TIMETYPE _timeout = 0;
    TIMETYPE _nexttime = 0;
    callback _handler = {};
    void* _param = nullptr;
    int _repeats = 0;
    uint8_t _state = TIMER_STATE_NONE;

    timer_node* _prev = nullptr;
    timer_node* _next = nullptr;
};

struct timerlist
{
    timerlist() : head(nullptr), count(0) {}
    void push_back(timer_node* t)
    {
        if(head)
        {
            ASSERT(head->_prev != nullptr);
            t->_prev = head->_prev;
            t->_next = head;
            head->_prev->_next = t;
            head->_prev = t;
        }
        else
        {
            t->_prev = t;
            t->_next = t;
            head = t;
        }
        ++count;
    }
    void pop(timer_node* t)
    {
        ASSERT(head);
        if(t == head && count == 1)
        {
            head = nullptr;
            count = 0;
            return;
        }
        ASSERT(t->_next);
        t->_next->_prev = t->_prev;
        ASSERT(t->_prev);
        t->_prev->_next = t->_next;
        if(t == head)
        {
            ASSERT(t->_next);
            head = t->_next;
        }
        t->_prev = nullptr;
        t->_next = nullptr;
        --count;
    }
    void clear()
    {
        head = nullptr;
        count = 0;
    }
    void swap(timerlist& rhs)
    {
        std::swap(head,  rhs.head);
        std::swap(count, rhs.count);
    }
    timer_node* head = nullptr;
    size_t count = 0;
};

class timewheel : public singleton_support<timewheel>
{
public:
    void init();
    auto add_timer(bool delay, TIMETYPE timeout/*ms*/, int repeats, callback handler, void* param) -> TIMERID;
    bool del_timer(TIMERID timerid);
    void run();
    void stop();

    FORCE_INLINE uint64_t get_tickcount(){ return _tickcount; }
    FORCE_INLINE TIMETYPE get_ticktime() { return _ticktime;  }

private:
    void add_to_changelist(timer_node* t);
    void readd_timer(timer_node* t);
    void remove_timer(timer_node* t);
    void free_timer(timer_node* t);
    void load_timers();
    void handle_timer();
    void switch_changelist();
    void regroup_timers();
    void tick();

    FORCE_INLINE timerlist& get_front_changelist() { return _changelist[_frontidx];  }
    FORCE_INLINE timerlist& get_back_changelist()  { return _changelist[!_frontidx]; }

private:
    bool _stop = true;
    timerlist _near_slots[NEAR_SLOTS];
    timerlist _hanging_slots;

    uint64_t _tickcount = 0;
    TIMETYPE _ticktime  = 0; // ms

    bee::spinlock _changelist_locker;
    bool _frontidx = 0;
    timerlist _changelist[2];

    lockfree_objectpool<timer_node> _timerpool;
};

} // namespace bee