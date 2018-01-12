#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"
#include "token_bucket_limiter.h"

#include "mocks/mock_logging.c"
#include "mocks/mock_delay.c"

// simple test: burst limit at 4 events
void test_burst(void)
{
    delay_mock_init();
    TokenBucketLimiter limit;
    token_bucket_limiter_init(&limit, 1, 10*1e6, 4);

    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 5));
    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 4));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 1));
}

// simple test: only allow one event every 10 sec (plus burst)
void test_rate(void)
{
    delay_mock_init();
    TokenBucketLimiter limit;
    token_bucket_limiter_init(&limit, 1, 10*1e6, 4);

    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 4));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 1));
    delay_mock_add_micros(10*1e6);
    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 1));
}

// test burst is limited, even if a lot of time passes
void test_rate__burst_limited(void)
{
    delay_mock_init();
    TokenBucketLimiter limit;
    token_bucket_limiter_init(&limit, 1, 10*1e6, 4);

    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 4));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 1));
    delay_mock_add_micros(1000*1e6);
    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 1));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 4));
}

// test burst is limited, even if a lot of time passes
void test_rate__burst_limited2(void)
{
    delay_mock_init();
    delay_mock_add_micros(400*1e6);
    TokenBucketLimiter limit;
    token_bucket_limiter_init(&limit, 1, 10*1e6, 4);

    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 2));
    delay_mock_add_micros(40*1e6);
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 5));
    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 4));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 1));
}

// test if the rate is correct when polled at irregular intervalse
void test_rate__rounding(void)
{
    delay_mock_init();
    TokenBucketLimiter limit;
    token_bucket_limiter_init(&limit, 1, 10*1e6, 4);

    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 4));
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 1));
    delay_mock_add_micros(9*1e6);
    delay_mock_add_micros(9*1e6);
    // t = 18 sec (1 token)
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 2));
    delay_mock_add_micros(2.00001*1e6);
    // t = 20.00001 sec (2 tokens)
    TEST_ASSERT_FALSE(token_bucket_limiter_allowed(&limit, 3));
    TEST_ASSERT_TRUE(token_bucket_limiter_allowed(&limit, 2));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_burst);
    RUN_TEST(test_rate);
    RUN_TEST(test_rate__burst_limited);
    RUN_TEST(test_rate__burst_limited2);
    RUN_TEST(test_rate__rounding);

    UNITY_END();
    return 0;
}