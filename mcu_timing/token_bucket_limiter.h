#ifndef TOKEN_BUCKET_LIMITER_H
#define TOKEN_BUCKET_LIMITER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    
    // settings
    uint64_t num_req_per_interval;
    uint64_t max_tokens;
    unsigned int interval_us;

    // state
    unsigned int tokens;
    uint64_t timestamp;

} TokenBucketLimiter;

void token_bucket_limiter_init(TokenBucketLimiter* limiter,
        unsigned int max_requests,
        unsigned int interval_us,
        unsigned int max_burst);

/**
 * Check if a specified amount of events is allowed.
 *
 * The rate limiter either allows all the requested events (returns true)
 * or disallows them all (returns false).
 *
 * @param limiter       A struct containing all relevant state.
 *                      First initialize it with token_bucket_limiter_init()
 *
 * @param num_events    Amount of events to 'claim'. If the token bucket has
 *                      enough tokens left, the request is allowed.
 *                      If not enough tokens are available, the request is
 *                      ignored, but no tokens are removed from the bucket.
 *
 * @return              True if the request is allowed (rate limit not reached),
 *                      false if the limit is reached.
 */
bool token_bucket_limiter_allowed(TokenBucketLimiter* limiter,
        unsigned int num_events);

#endif
