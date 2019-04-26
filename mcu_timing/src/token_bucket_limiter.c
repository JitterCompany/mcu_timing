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

    limiter->available_tokens = limiter->max_tokens;
    limiter->micro_tokens = 0;
    limiter->timestamp = delay_get_timestamp();
}

/*
 * Every interval 'num_req_per_interval' tokens are added, up to max_burst.
 *
 * Implementation: instead of directly storing tokens,
 * they are first stored as multiple of the time interval:
 * 1 token = 'interval' microtokens.
 *
 * When enough microtokens are buffered to form a multiple of 'num_req_pre_interval',
 * they are converted to 'available_tokens'.
 * This approach allows precise calculations without depending on a fixed-frequency
 * update of the available tokens.
 */
static void update(TokenBucketLimiter *limiter)
{
    const unsigned int n_per_req = limiter->num_req_per_interval;
    const uint64_t scale_factor = limiter->interval_us;

    // add 'num_req_per_interval' new micro_tokens every 'interval_us' microseconds
    uint64_t now = delay_get_timestamp();
    uint64_t new_micros = delay_calc_time_us(limiter->timestamp, now);
    const uint64_t new_u_tokens = new_micros * n_per_req;
    limiter->timestamp = now;
       
    const uint64_t max_new = ((limiter->max_tokens - limiter->available_tokens)
        * scale_factor);

    if(new_u_tokens > max_new) {
        limiter->micro_tokens = max_new;

    } else if((max_new - new_u_tokens) < limiter->micro_tokens) {
        limiter->micro_tokens = max_new;

    } else {
        limiter->micro_tokens+= new_u_tokens;
    }

    // Normalize: move any multiple of 'n_per_req' micro_tokens from
    // the microtokens buffer to the available_tokens
    const unsigned int scaled_n_per_req = n_per_req * scale_factor;
    const unsigned int available = n_per_req * (limiter->micro_tokens / (scaled_n_per_req));
    limiter->micro_tokens-= (available * scale_factor);
    limiter->available_tokens+= available;
}



bool token_bucket_limiter_allowed(TokenBucketLimiter* limiter,
        unsigned int num_events)
{

    update(limiter);

    if(limiter->available_tokens < num_events) {
        return false;
    }

    limiter->available_tokens-= num_events;
    return true;
}

unsigned int token_bucket_limiter_count_available(TokenBucketLimiter* limiter)
{
    update(limiter);

    return limiter->available_tokens;
}

