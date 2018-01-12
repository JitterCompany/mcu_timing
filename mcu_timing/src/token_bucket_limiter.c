#include "token_bucket_limiter.h"
#include "delay.h"
#include <c_utils/max.h>

void token_bucket_limiter_init(TokenBucketLimiter* limiter,
        unsigned int max_requests,
        unsigned int interval_us,
        unsigned int max_burst)
{
    limiter->num_req_per_interval = max_requests;
    limiter->interval_us = interval_us;
    limiter->max_tokens = max_burst;

    limiter->remainder = limiter->max_tokens;
    limiter->tokens = 0;
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
    const unsigned int n_per_req = limiter->num_req_per_interval;
    const uint64_t interval_us = limiter->interval_us;

    // add 'num_req_per_interval' new tokens every 'interval_us' microseconds
    uint64_t now = delay_get_timestamp();
    uint64_t new_micros = delay_calc_time_us(limiter->timestamp, now);
    const uint64_t new_u_tokens = new_micros * n_per_req;
    limiter->timestamp = now;
       
    const uint64_t max_new = ((limiter->max_tokens - limiter->remainder)
        * interval_us);

    if(new_u_tokens > max_new) {
        limiter->tokens = max_new;

    } else if((max_new - new_u_tokens) < limiter->tokens) {
        limiter->tokens = max_new;

    } else {
        limiter->tokens+= new_u_tokens;
    }

    // First claim events from the remainder
    unsigned int claim_remain = min(limiter->remainder, num_events);
    num_events-= claim_remain; 
   
    // All required events could be claimed: succes! 
    if(!num_events) {
        limiter->remainder-= claim_remain;
        return true;
    }

    // Some 'extra' tokens should be in the bucket even if they are not used.
    // This simulates adding n_per_req tokens at once every interval, instead
    // of adding one token every (interval/n_per_req).
    unsigned int extra = num_events % n_per_req;
    uint64_t extra_required = 0;
    if(extra) {
        extra_required = ((n_per_req - extra)) * interval_us;
    }
    const uint64_t required = (uint64_t)num_events * interval_us;

    // Check if we have enough microtokens
    if((required + extra_required) > limiter->tokens) {
        return false;
    }
    limiter->tokens -= required;
    limiter->remainder-= claim_remain;
    return true;
}

