#ifndef RATE_LIMIT_H
#define RATE_LIMIT_H

#include <stdint.h>
#include <stdbool.h>
#include <mcu_timing/delay.h>

typedef struct {
    uint64_t min_delay;
    uint64_t max_delay;
    uint64_t treshold_delay;

    uint64_t delay;
    uint32_t inc_counter;
    uint32_t inc_max;
    bool increase;

    delay_timeout_t timeout;
    delay_timeout_t treshold_timeout;
} RateLimit;

/* Rate limit: limit the rate of a certain action if it is tried
 * faster than the limit.
 *
 * Every time a request is allowed, the delay is adjusted.
 * If a request was attempted during the first 'treshold_delay' microseconds,
 * a counter is incremented. If that counter reaches 'up_treshold',
 * the delay is doubled.
 * If no request was attempted during the first 'treshold_delay' microseconds,
 * the delay is halved.
 * The delay is capped/limited to min_delay and max_delay.
 */
void rate_limit_init(RateLimit *limit, uint64_t min_delay, uint64_t max_delay,
       uint64_t treshold_delay, uint32_t up_treshold);

// return true if current request is allowed
bool rate_limit_allowed(RateLimit *limit);

#endif

