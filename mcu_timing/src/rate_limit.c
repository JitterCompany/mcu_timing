#include "rate_limit.h"

void rate_limit_init(RateLimit *limit, uint64_t min_delay, uint64_t max_delay,
       uint64_t treshold_delay, uint32_t up_treshold)
{
    limit->min_delay = min_delay;
    limit->max_delay = max_delay;
    limit->treshold_delay = treshold_delay;

    limit->delay = min_delay;
    limit->increase = false;
    limit->inc_counter = 0;
    limit->inc_max = up_treshold;

    delay_timeout_set(&limit->timeout, 0);
    delay_timeout_set(&limit->treshold_timeout, 0);
}

bool rate_limit_allowed(RateLimit *limit)
{
    if(!delay_timeout_done(&limit->treshold_timeout)) {
        if(!limit->increase) {
            limit->increase = true;
        }
    }
    // waited long enough
    if(delay_timeout_done(&limit->timeout)) {
        uint64_t delay = limit->delay;
        if(limit->increase) {
            limit->increase = false;
            limit->inc_counter++;
            if(limit->inc_counter >= limit->inc_max) {
                limit->inc_counter = 0;
                delay = delay * 2;
            }
        } else {
            delay = delay / 2;
        }
        if(delay < limit->min_delay) {
           delay = limit->min_delay;
        }
        if(delay > limit->max_delay) {
            delay = limit->max_delay;
        }
        limit->delay = delay;

        delay_timeout_set(&limit->timeout, limit->delay);
        delay_timeout_set(&limit->treshold_timeout, limit->treshold_delay);
        return true;
    }

    return false;
}

