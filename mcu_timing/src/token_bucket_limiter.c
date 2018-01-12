#include "token_bucket_limiter.h"
#include "delay.h"

void token_bucket_limiter_init(TokenBucketLimiter* limiter,
        unsigned int max_requests,
        unsigned int interval_us,
        unsigned int max_burst)
{
    limiter->num_req_per_interval = max_requests;
    limiter->interval_us = interval_us;
    limiter->max_tokens = max_burst * interval_us;

    limiter->tokens = limiter->max_tokens;
    limiter->timestamp = delay_get_timestamp();
}

/*
 * Every interval 'num_req_per_interval' tokens are added, up to max_burst.
 *
 * Implementation: instead of storing tokens, they are stored as multiple
 * of the time interval: 1 token = 'interval' microtokens.
 * 
 * Internally all calculations are based on these 'microtokens' to make
 * calculation of time differences easier & more efficient.
 */
bool token_bucket_limiter_allowed(TokenBucketLimiter* limiter,
        unsigned int num_events)
{
    // add 'num_req_per_interval' new tokens every 'interval_us' microseconds
    uint64_t now = delay_get_timestamp();
    uint64_t new_micros = delay_calc_time_us(limiter->timestamp, now);
    const uint64_t new_tokens = new_micros * limiter->num_req_per_interval;
    limiter->timestamp = now;
       
    if(new_tokens > limiter->max_tokens) {
        limiter->tokens = limiter->max_tokens;

    } else if((limiter->max_tokens - new_tokens) < limiter->tokens) {
        limiter->tokens = limiter->max_tokens;

    } else {
        limiter->tokens+= new_tokens;
    }

    const uint64_t required_tokens = (
            ((uint64_t)num_events) * limiter->interval_us);
    if(required_tokens <= limiter->tokens) {
        limiter->tokens -= required_tokens;
        return true;
    }
    return false;
}

