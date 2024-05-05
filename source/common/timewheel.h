#pragma once
#include <cstdint>
#include <functional>
#include "lock.h"
#include "types.h"
#include "util.h"
#include "objectpool.h"

#define NEAR_SLOTS 1024
constexpr static size_t TIMERPOOL_SIZE = 1 << 12;

using callback = std::function<bool(void*)>;

enum TIMER_OBJECT_STATE
{
    TIMER_STATE_NONE,
    TIMER_STATE_ACTIVE,
    TIMER_STATE_ADD,
    TIMER_STATE_DEL,
    TIMER_STATE_MOD,
};

struct timer_node : public light_object_base<cassobee::spinlock>
{
    void assign(uint64_t id, TIMETYPE timeout, TIMETYPE nexttime, callback handler, void* param, int repeats, uint8_t state)
    {
        _id = id;
        _timeout = timeout;
        _nexttime = nexttime;
        _handler = handler;
        _param = param;
        _repeats = repeats;
        _state = state;
        _prev = nullptr;
        _next = nullptr;
    }

    uint64_t _id = 0;
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
    void push_front(timer_node* t)
    {
        if(head)
        {
            t->_prev = head->_prev;
            t->_next = head;
            if(head->_prev) head->_prev->_next = t;
            head->_prev = t;
        }
        else
        {
            t->_prev = t;
            t->_next = head;
        }
        head = t;
        ++count;
    }
    void push_back(timer_node* t)
    {
        if(head)
        {
            t->_prev = head->_prev;
            t->_next = head;
            if(head->_prev) head->_prev->_next = t;
            head->_prev = t;
        }
        else
        {
            t->_prev = t;
            t->_next = head;
            head = t;
        }
        ++count;
    }
    void pop(timer_node* t)
    {
        if(t->_prev) t->_prev->_next = t->_next;
        if(t->_next) t->_next->_prev = t->_prev;
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
    void init(TIMETYPE ticktime);
    int  add_timer(bool delay, TIMETYPE timeout, int repeats, callback handler, void* param);
    bool del_timer(int timerid);
    void run();

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
    timerlist _near_slots[NEAR_SLOTS];
    timerlist _hanging_slots;

    uint64_t _tickcount = 0;
    TIMETYPE _ticktime  = 0; // ms

    cassobee::spinlock _changelist_locker;
    bool _frontidx = 0;
    timerlist _changelist[2];

    objectpool<timer_node> _timerpool;
};