#ifndef INTERVAL_H
#define INTERVAL_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_INTERVALS 5

typedef void (*IntervalCB)(void);

typedef struct {
    uint32_t time;
    IntervalCB cb;
    bool reached;
} Interval;

typedef struct {
    Interval intervals[MAX_INTERVALS];
    int num_intervals;
    uint32_t last_time;
    uint32_t counter;
} IntervalList;

/**
 * Initialize a IntervalList data structure
 */
void interval_init(IntervalList *interval_list);

/**
 * Add an interval to the intervallist
 *
 * time: the interval time in a arbitrary time unit. Make sure the same unit is 
 * used in interval_irq_handler.
 * cb: a callback function that needs to get called every time the interval 
 * has passed.
 */
bool interval_add(IntervalList *interval_list, uint32_t time, IntervalCB cb);

/**
 * Call this function in your main loop.
 * This will call all callbacks that are due.
 */
void interval_poll(IntervalList *interval_list);

/**
 * update the interval list with the current time using the same time units 
 * as in interval add. 
 * For example, if a interval is configured for 5 seconds, this function needs 
 * to be called every second with the current time in seconds in order not 
 * to miss a callback.
 */
void interval_irq_handler(IntervalList *interval_list, uint32_t time);

#endif
