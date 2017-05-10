#include "interval.h"

void interval_init(IntervalList *interval_list)
{
    interval_list->num_intervals = 0;
    interval_list->last_time = 0;
    interval_list->counter = 0;
    interval_list->poll_required = false;
}

bool interval_add(IntervalList *interval_list, uint32_t time, IntervalCB cb)
{
    if (interval_list->num_intervals < MAX_INTERVALS) {
        Interval *interval = 
            &interval_list->intervals[interval_list->num_intervals];
        interval->time = time;
        interval->cb = cb;
        interval->reached = false;
        interval_list->num_intervals += 1;
        return true;
    }
    return false;
}

bool interval_is_poll_required(IntervalList *interval_list)
{
    return interval_list->poll_required;
}

void interval_poll(IntervalList *interval_list)
{
    // immediately set to false: if an IRQ happens during this call,
    // poll_required may be set to true again
    interval_list->poll_required = false;

    for(int i=0; i < interval_list->num_intervals; i++) {
        Interval *interval = 
            &interval_list->intervals[i];
        if (interval->reached && interval->cb) {
            interval->reached = false;
            interval->cb();
        }
    }
}

void interval_irq_handler(IntervalList *interval_list, uint32_t time)
{
    if(!time || (time == interval_list->last_time)) {
        return;
    }
    interval_list->last_time = time;
    interval_list->counter += 1;

    for(int i=0; i < interval_list->num_intervals; i++) {
        Interval *interval = 
            &interval_list->intervals[i];
        if (!(interval_list->counter % interval->time)) {
            interval->reached = true;
            interval_list->poll_required = true;
        }
    }
}
