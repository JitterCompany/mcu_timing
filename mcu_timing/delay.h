#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t target_timestamp;
} delay_timeout_t;


// If you have DELAY_SHARE_TIMER=1 set in cmake, you should set DELAY_OWNER=1
// for the core that 'owns' the delay. Only this core should init(), deinit() etc.
// By default, this feature is disabled and each core 'owns' its own timer.
#if (!defined(DELAY_SHARE_TIMER))
    #define DELAY_SHARE_TIMER (0)
#endif
#if (!defined(DELAY_OWNER))
    #if (!DELAY_SHARE_TIMER)
        #define DELAY_OWNER (1)
    #else
        #define DELAY_OWNER (0)
    #endif
#endif


#if (DELAY_OWNER)

/**
 * Initialize the delay timer.
 *
 * Call this function before using any other delay_ functions.
 * This enables an internal timer + interrupt and initializes internal state.
 *
 * Note: on multicore processors, each core is typically assigned its own timer peripheral.
 * To share the same timer between cores, define DELAY_SHARE_TIMER in CMake.
 */
void delay_init(void);

/**
 * Re-initialize the delay timer with a given initial timestamp.
 *
 * Use this function if the delay timer was de-initialized for some time,
 * for example during a deep sleep state.
 *
 * All other functions such as delay_get_timestamp() or delay_timeout_done()
 * will behave as if the timer has continued ticking to the given timestamp.
 */
void delay_reinit(uint64_t initial_timestamp);


/**
 * De-initialized the delay timer.
 *
 * This stops the internal timer, disables the internal timer IRQ and
 * resets all internal state. After de-initializing,
 * call delay_init() again before calling any other delay_ function.
 */
void delay_deinit(void);

#endif



/* Get a timestamp (unit is ticks since startup).
 *
 * Use the result for delay_calc_time_us to convert to microseconds
 */
uint64_t delay_get_timestamp(void);

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
