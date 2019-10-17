#include "delay.h"
#include "chip.h"
#include <lpc_tools/irq.h>
#include <c_utils/assert.h>
#include <string.h>

#define TIMER_HALFWAY      (0x80000000)

//
// Platform specific code
//
#if (!defined(DELAY_MEMORY_SECTION))
    #if (DELAY_SHARE_TIMER)
        #error When sharing the timer between cpu cores, DELAY_MEMORY_SECTION \
            should be defined! \
            If this is not a multi-CPU setup, disable DELAY_SHARE_TIMER!
    #endif
    #define SECTION_STATEMENT
#else
    #define SECTION_STATEMENT   __attribute__((section(DELAY_MEMORY_SECTION)))
#endif

#if (defined(MCU_PLATFORM_43xx_m4) || defined(MCU_PLATFORM_43xx_m0))

    #if (DELAY_SHARE_TIMER)
        #define DELAY_TIMER             LPC_TIMER3

        #if (DELAY_OWNER)
            #define DELAY_TIMER_IRQn        TIMER3_IRQn
            #define DELAY_IRQHandler        TIMER3_IRQHandler

            #define DELAY_RGU_TIMER_RST     RGU_TIMER3_RST

            #define DELAY_CLOCK             CLK_MX_TIMER3
        #endif

    #elif defined(MCU_PLATFORM_43xx_m0)
        #define DELAY_TIMER             LPC_TIMER3
        #define DELAY_TIMER_IRQn        TIMER3_IRQn
        #define DELAY_IRQHandler        TIMER3_IRQHandler

        #define DELAY_RGU_TIMER_RST     RGU_TIMER3_RST

        #define DELAY_CLOCK             CLK_MX_TIMER3

    #elif (defined(MCU_PLATFORM_43xx_m4))
        #define DELAY_TIMER             LPC_TIMER2
        #define DELAY_TIMER_IRQn        TIMER2_IRQn
        #define DELAY_IRQHandler        TIMER2_IRQHandler

        #define DELAY_RGU_TIMER_RST     RGU_TIMER2_RST

        #define DELAY_CLOCK             CLK_MX_TIMER2

    #endif

#if (DELAY_OWNER)
static inline void reset_timer(void)
{
    Chip_RGU_TriggerReset(DELAY_RGU_TIMER_RST);
    while (Chip_RGU_InReset(DELAY_RGU_TIMER_RST)) {}
}
static inline uint32_t get_timer_clock_rate(void)
{
    return Chip_Clock_GetRate(DELAY_CLOCK);
}
#endif

#elif defined(MCU_PLATFORM_lpc11xxx)
    #define DELAY_TIMER             LPC_TIMER32_0
    #define DELAY_TIMER_IRQn        TIMER_32_0_IRQn
    #define DELAY_IRQHandler        TIMER32_0_IRQHandler

#if (DELAY_OWNER)
static inline void reset_timer(void){}
static inline uint32_t get_timer_clock_rate(void)
{
    return Chip_Clock_GetMainClockRate();
}
#endif

#else
    #error "the current platform is not supported yet"
    
    /* To port to a new chip family, define these symbols:
    DELAY_TIMER
    DELAY_TIMER_IRQn
    DELAY_IRQHandler

    static inline void reset_timer(void);
    static inline uint32_t get_timer_clock_rate(void);
    */
#endif

/**
 * past_halfway     is a flag that is set whenever the timer value is known to
 * be above TIMER_HALFWAY. This is used to detect overflow: if the timer reads
 * as a small value while this flag is set, the timer has overflowed.
 * This allows the irqhandler to run on a different core and under any
 * priority level, as long as it is handled with latency << TIMER_HALFWAY microseconds
 *
 * overflow_count   counts the amount of times the 32-bit timer has overflowed.
 * Together with the 32-bit timer value this forms a 64-bit timer.
 */
typedef struct {
    volatile bool past_halfway;
    volatile uint32_t overflow_count;
} TimeInfo;

/**
 * index    IRQ toggles this between 1 and 0: g_state.time[index] always
 * contains a consistent set of data. This allows the DELAY_TIMER_IRQ to run
 * at a low priority: any high-priority IRQ can interrupt it
 * and read a consistent set of time info from the last index.
 *
 * NOTE: it might even be possible to use 'index' as a 'past_halfway' signal,
 * as both normally toggle at the same time. But this might sligtly complicate
 * delay_reinit()..
 */
static struct {
    TimeInfo time[2];
    volatile bool index;

} g_state SECTION_STATEMENT;


#if (DELAY_OWNER)
static void timer_init(uint32_t offset_ticks)
{
    // Enable timer clock and reset it
    Chip_TIMER_Init(DELAY_TIMER);
    reset_timer();
    Chip_TIMER_Reset(DELAY_TIMER);

    // run timer at 1Mhz
    const uint32_t cpu_freq_MHz = get_timer_clock_rate() / 1000000;
    Chip_TIMER_PrescaleSet(DELAY_TIMER, cpu_freq_MHz-1);

    // interrupt on overflow (2^32 microseconds)
    // Match 1: interrupt to handle overflow
    Chip_TIMER_MatchEnableInt(DELAY_TIMER, 1);
    Chip_TIMER_SetMatch(DELAY_TIMER, 1, 0xFFFFFFFF);
    Chip_TIMER_ResetOnMatchDisable(DELAY_TIMER, 1);

    // Match 2: interrupt to make overflow detection easy on multicore systems
    Chip_TIMER_MatchEnableInt(DELAY_TIMER, 2);
    Chip_TIMER_SetMatch(DELAY_TIMER, 2, TIMER_HALFWAY);
    Chip_TIMER_ResetOnMatchDisable(DELAY_TIMER, 2);

    DELAY_TIMER->TC = offset_ticks;

    // Enable timer interrupt
    NVIC_ClearPendingIRQ(DELAY_TIMER_IRQn);
    NVIC_EnableIRQ(DELAY_TIMER_IRQn);

    Chip_TIMER_Enable(DELAY_TIMER);
}

static void timer_deinit(void)
{
    Chip_TIMER_DeInit(DELAY_TIMER);

    NVIC_DisableIRQ(DELAY_TIMER_IRQn);
    NVIC_ClearPendingIRQ(DELAY_TIMER_IRQn);
}

void DELAY_IRQHandler(void)
{
    const bool new_index = !(g_state.index);
    TimeInfo *new_time = &g_state.time[new_index];

    const uint32_t ovf_count = (g_state.time[g_state.index].overflow_count);

    if (Chip_TIMER_MatchPending(DELAY_TIMER, 1)) {
        Chip_TIMER_ClearMatch(DELAY_TIMER, 1);

        // Overflow: increment overflow_count
        new_time->overflow_count = ovf_count + 1;
        new_time->past_halfway = false;
    }

    if (Chip_TIMER_MatchPending(DELAY_TIMER, 2)) {
        Chip_TIMER_ClearMatch(DELAY_TIMER, 2);

        // Timer is halfway: set 'past_halfway' flag
        new_time->overflow_count = ovf_count;
        new_time->past_halfway = true;
    }

    __DMB();
    g_state.index = new_index;
}

void delay_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    timer_init(0);
}

void delay_deinit(void)
{
    timer_deinit();
}

void delay_reinit(uint64_t initial_timestamp)
{
    const uint32_t hi_offset = (initial_timestamp >> 32);
    const uint32_t lo_offset = (initial_timestamp & 0xFFFFFFFF);

    const bool new_index = !(g_state.index);
    TimeInfo *time = &g_state.time[new_index];
    __DMB();

    time->overflow_count = hi_offset;
    time->past_halfway = (lo_offset >= TIMER_HALFWAY);

    __DMB();
    g_state.index = new_index;

    timer_init(lo_offset);
}
#endif



//
// Shared code below: these functions are accessible from all contexts
//


uint64_t delay_get_timestamp()
{
    uint32_t hi_count;
    uint32_t lo_count;

    // The 64-bit count is created from two parts:
    // * The lower 32-bits are directly from a 32-bit hardware timer
    // * The higher bits are incremented by an interrupt whien the lower
    //      bits overflow.
    //
    // The loop ensures that a consistent combinations of the two counts
    // is used.
    bool index;
    do { 
        index = g_state.index;
        const TimeInfo *time = &g_state.time[index];
        __DMB();

        hi_count = time->overflow_count;
        lo_count = DELAY_TIMER->TC;

        // Detect timer overflow in case we run on a different cpu core from
        // the IRQ handler (or from a higher irq priority)
        if(time->past_halfway && (lo_count < TIMER_HALFWAY)) {
            hi_count+=1;
        }
    
        // Repeat if state was changed in between (e.g. IRQ is/was active)
        __DMB();
    } while (index != g_state.index);

    return (((uint64_t)hi_count) << 32) | lo_count;
}

uint64_t delay_calc_time_us(uint64_t start_timestamp, uint64_t end_timestamp)
{
    if(start_timestamp > end_timestamp) {
        return 0;
    }

    return end_timestamp - start_timestamp;
}

void delay_us(uint64_t microseconds)
{
    delay_timeout_t timeout;
    delay_timeout_set(&timeout, microseconds);
    while(!delay_timeout_done(&timeout)) {}
}

void delay_timeout_set(delay_timeout_t *timeout, uint64_t microseconds)
{
    timeout->target_timestamp = delay_get_timestamp() + microseconds;
}

bool delay_timeout_done(delay_timeout_t *timeout)
{
    return (delay_get_timestamp() >= timeout->target_timestamp);
}

// Delay using a loop (deprecated).
void delay_loop_us(uint32_t clk_freq, uint32_t us)
{
    uint32_t wait = clk_freq * (us / 1000000.0);
    volatile uint32_t i = 0;
    while (i++ < wait);
}

