#include <stdint.h>
#include <stdbool.h>
#include "delay.h"

static struct {
    uint64_t time_3_us;
} g_state;

void delay_mock_init(void) {
    g_state.time_3_us = 0;
}

void delay_mock_add_micros(uint64_t microseconds) {
    log_info("time was %u micros", g_state.time_3_us/3);
    log_info("add %u micros", microseconds);
    g_state.time_3_us+= 3*microseconds;
    log_info("time is now %u micros", g_state.time_3_us/3);
}

void delay_mock_add_seconds(uint32_t seconds) {
    uint64_t seconds_u64 = seconds;
    uint64_t micros_u64 = seconds_u64 * 1000000;
    delay_mock_add_micros(micros_u64);
}

uint64_t delay_get_timestamp()
{
    return g_state.time_3_us;
}

bool delay_timeout_done(delay_timeout_t *timeout)
{
    log_info("delay_timeout_done(): time is %u (%u) micros, target is %u",
            g_state.time_3_us/3,
            delay_get_timestamp(),
            timeout->target_timestamp);
    bool done = (delay_get_timestamp() >= timeout->target_timestamp);
    log_info("done=%d", (int)done);
    return done;
}

void delay_timeout_set(delay_timeout_t *timeout, uint64_t microseconds)
{
    timeout->target_timestamp = g_state.time_3_us + 3*microseconds;
}

uint64_t delay_calc_time_us(uint64_t start_timestamp, uint64_t end_timestamp)
{
    if(start_timestamp > end_timestamp) {
        return 0;
    }

    return (end_timestamp - start_timestamp) / 3;
}

