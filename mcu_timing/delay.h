#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t target_timestamp;
} delay_timeout_t;

/* Initialize the internal timer.
 */
void delay_init();

/* Get a timestamp (unit is ticks since startup).
 *
 * Use the result for delay_calc_time_us to convert to microseconds
 */
uint64_t delay_get_timestamp();

/* Calculate the time diference in microseconds between two timestamps
 *
 * @param start_timestamp       timestamp from delay_get_timestamp()
 *                              at the start time
 * @param end_timestamp         timestamp from delay_get_timestamp()
 *                              at the end time
 *
 * @return                      time difference in microseconds.
 *                              Note: this assumes the clock frequency is
 *                              the same since start_timestamp
 */
uint64_t delay_calc_time_us(uint64_t start_timestamp, uint64_t end_timestamp);

/* Delay using an internal timer.
 * This is very precise, as long as the clock frequency stays the same.
 * When changing the clock frequency, re-initialize the delay timer with
 * delay_init()
 *
 * @param us            amount of microseconds to block
 */
void delay_us(uint64_t microseconds);

/* Set a non-blocking timeout.
 * This offers more flexibility than delay_us.
 *
 * @param timeout       Timeout struct to be initialized by this function.
 *                      An arbitrary amount of these structs may be created
 *                      to have multiple simultaneous timers, as the struct
 *                      holds all relevant state
 *
 * @param microseconds  Amount of microseconds after which the timeout is done.
 *                      Periodically poll with delay_timeout_done().
 */
void delay_timeout_set(delay_timeout_t *timeout, uint64_t microseconds);

/* Check if a running timeout is done
 *
 * @param timeout       Timeout struct earlier initialized by
 *                      delay_timeout_set()
 *
 * @return              True if the timeout is done, False if not (yet)
 */
bool delay_timeout_done(delay_timeout_t *timeout);


/* Delay using a loop (deprecated).
 * Not very precise, but does not depend on timers.
 * Only works up to 20 seconds.
 * For more precise timing, see delay_us() and delay_timeout_set()
 *
 * @param clk_freq      CPU frequency in HZ
 * @param us            amount of microseconds to block
 */
void delay_loop_us(uint32_t clk_freq, uint32_t us);


#endif
