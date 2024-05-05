#include <cstdio>
#include <unistd.h>
#include "timewheel.h"

void timewheel::init(TIMETYPE ticktime)
{
    _ticktime = ticktime;
    _timerpool.init(TIMERPOOL_SIZE);
}

int timewheel::add_timer(bool delay, TIMETYPE timeout, int repeats, callback handler, void* param)
{
    if(!handler) return -1;
    auto [timerid, t] = _timerpool.alloc();
    if(!t) return -1;
    t->assign(timerid, timeout * 1000 / _ticktime, delay ? _tickcount + t->_timeout : _tickcount, handler, param, repeats, TIMER_STATE_ADD);
    add_to_changelist(t);
    return timerid;
}

bool timewheel::del_timer(int timerid)
{
    if(timer_node* t = _timerpool.find_object(timerid))
    {
        cassobee::spinlock::scoped l(t->_locker);
        if(t->_state != TIMER_STATE_ACTIVE) return false; // 只支持删除active状态的定时器
        t->_state = TIMER_STATE_DEL;
        add_to_changelist(t);
        printf("del_timer %lu\n", t->_id);
    }
    return false;
}

void timewheel::add_to_changelist(timer_node* t)
{
    cassobee::spinlock::scoped l(_changelist_locker);
    get_back_changelist().push_back(t);
}

void timewheel::readd_timer(timer_node* t)
{
    if(t->_nexttime - _tickcount >= NEAR_SLOTS)
    {
        _hanging_slots.push_back(t);
        printf("readd_timer %lu _nexttime=%ld in _hanging_slots, _tickcount=%ld\n", t->_id, t->_nexttime, _tickcount);
    }
    else
    {
        int offset = _tickcount % NEAR_SLOTS;
        if(t->_nexttime - _tickcount > 0)
            offset = t->_nexttime % NEAR_SLOTS;
        _near_slots[offset].push_back(t);
        printf("readd_timer %lu _nexttime=%ld in _near_slots %d, _tickcount=%ld\n", t->_id, t->_nexttime, offset, _tickcount);
    }
    t->_state = TIMER_STATE_ACTIVE;
}

void timewheel::remove_timer(timer_node* t)
{
    if(t->_nexttime - _tickcount >= NEAR_SLOTS)
    {
        _hanging_slots.pop(t);
        printf("remove_timer %lu in _hanging_slots\n", t->_id);
    }
    else
    {
        int offset = _tickcount % NEAR_SLOTS;
        if(t->_nexttime - _tickcount > 0)
            offset = (t->_nexttime - _tickcount) % NEAR_SLOTS;
        _near_slots[offset].pop(t);
        printf("remove_timer %lu in _near_slots %d\n", t->_id, offset);
    }
    free_timer(t);
}

void timewheel::free_timer(timer_node* t)
{
    t->_state = TIMER_STATE_NONE;
    _timerpool.free(t->_id);
    printf("free_timer %lu\n", t->_id);
}

void timewheel::load_timers()
{
    timerlist& changelist = get_front_changelist();
    size_t i = 0, count = changelist.count;
    for(timer_node* t = changelist.head; i < count; ++i)
    {
        cassobee::spinlock::scoped l(t->_locker);
        timer_node* next = t->_next;
        if(t->_state == TIMER_STATE_ADD)
        {
            readd_timer(t);
        }
        else if(t->_state == TIMER_STATE_DEL)
        {
            remove_timer(t);
        }
        else if(t->_state == TIMER_STATE_MOD)
        {
            readd_timer(t);
        }
        t = next;
    }
    changelist.clear();
}

void timewheel::handle_timer()
{
    int offset = _tickcount % NEAR_SLOTS;
    //printf("handle_timer offset=%d\n", offset);
    timerlist active_list;
    active_list.swap(_near_slots[offset]);
    size_t i = 0, count = active_list.count;
    for(timer_node* t = active_list.head; i < count; ++i)
    {
        cassobee::spinlock::scoped l(t->_locker);
        timer_node* next = t->_next;
        if(t->_state == TIMER_STATE_ACTIVE)
        {
            if(t->_handler(t->_param))
            {
                active_list.pop(t);
                if(t->_repeats > 0){ --(t->_repeats); }
                if(t->_repeats == 0)
                {
                    free_timer(t);
                }
                else
                {
                    t->_nexttime += t->_timeout;
                    readd_timer(t);
                }
            }
            else
            {
                free_timer(t);
            }
        }
        t = next;
    }
}

void timewheel::switch_changelist()
{
    cassobee::spinlock::scoped l(_changelist_locker);
    _frontidx = !_frontidx;
}

void timewheel::regroup_timers()
{
    size_t i = 0, count = _hanging_slots.count;
    for(timer_node* t = _hanging_slots.head; i < count; ++i)
    {
        cassobee::spinlock::scoped l(t->_locker);
        timer_node* next = t->_next;
        if(t->_nexttime - _tickcount < NEAR_SLOTS)
        {
            int offset = _tickcount % NEAR_SLOTS;
            if(t->_nexttime - _tickcount > 0)
                offset = t->_nexttime % NEAR_SLOTS;
            _near_slots[offset].push_back(t);
            _hanging_slots.pop(t);
        }
        t = next;
    }
}

void timewheel::tick()
{
    switch_changelist();
    load_timers();
    handle_timer();
    ++_tickcount;
    if(_tickcount % NEAR_SLOTS == 0)
    {
        regroup_timers();
    }
}

void timewheel::run()
{
    while(true)
    {
        tick();
        usleep(_ticktime * 1000);
    }
}
