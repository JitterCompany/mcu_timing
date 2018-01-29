#include "delay.h"
#include "chip.h"
#include <lpc_tools/irq.h>
#include <string.h>

//
// Platform specific code
//
#if (defined(MCU_PLATFORM_43xx_m4) || defined(MCU_PLATFORM_43xx_m0))

    #if defined(MCU_PLATFORM_43xx_m4)
        #define DELAY_TIMER             LPC_TIMER2
        #define DELAY_TIMER_IRQn        TIMER2_IRQn
        #define DELAY_IRQHandler        TIMER2_IRQHandler

        #define DELAY_RGU_TIMER_RST     RGU_TIMER2_RST

        #define DELAY_CLOCK             CLK_MX_TIMER2
    #endif

    #if defined(MCU_PLATFORM_43xx_m0)
        #define DELAY_TIMER             LPC_TIMER3
        #define DELAY_TIMER_IRQn        TIMER3_IRQn
        #define DELAY_IRQHandler        TIMER3_IRQHandler

        #define DELAY_RGU_TIMER_RST     RGU_TIMER3_RST

        #define DELAY_CLOCK             CLK_MX_TIMER3
    #endif

static inline void reset_timer(void)
{
    Chip_RGU_TriggerReset(DELAY_RGU_TIMER_RST);
    while (Chip_RGU_InReset(DELAY_RGU_TIMER_RST)) {}
}
static inline uint32_t get_timer_clock_rate(void)
{
    return Chip_Clock_GetRate(DELAY_CLOCK);
}

#elif defined(MCU_PLATFORM_11uxx)
    #define DELAY_TIMER             LPC_TIMER32_0
    #define DELAY_TIMER_IRQn        TIMER_32_0_IRQn
    #define DELAY_IRQHandler        TIMER32_0_IRQHandler

static inline void reset_timer(void){}
static inline uint32_t get_timer_clock_rate(void)
{
    return Chip_Clock_GetMainClockRate();
}

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



static struct {
    volatile uint32_t interrupt_count;
} g_state;



static void timer_init(const uint32_t cpu_freq_mhz)
{
    // Enable timer clock and reset it
    Chip_TIMER_Init(DELAY_TIMER);
    reset_timer();

    // run timer at 1Mhz
    Chip_TIMER_Reset(DELAY_TIMER);

    Chip_TIMER_PrescaleSet(DELAY_TIMER, cpu_freq_mhz-1);

    // interrupt on overflow (2^32 microseconds)
    Chip_TIMER_MatchEnableInt(DELAY_TIMER, 1);
    Chip_TIMER_SetMatch(DELAY_TIMER, 1, 0xFFFFFFFF);
    Chip_TIMER_ResetOnMatchDisable(DELAY_TIMER, 1);

    Chip_TIMER_Enable(DELAY_TIMER);

    // Enable timer interrupt
    NVIC_EnableIRQ(DELAY_TIMER_IRQn);
    NVIC_ClearPendingIRQ(DELAY_TIMER_IRQn);
}

static void timer_deinit()
{
    Chip_TIMER_DeInit(DELAY_TIMER);

    NVIC_DisableIRQ(DELAY_TIMER_IRQn);
    NVIC_ClearPendingIRQ(DELAY_TIMER_IRQn);
}

void DELAY_IRQHandler(void)
{
    if (Chip_TIMER_MatchPending(DELAY_TIMER, 1)) {
        Chip_TIMER_ClearMatch(DELAY_TIMER, 1);

        g_state.interrupt_count++;
    }
}

void delay_init()
{
    g_state.interrupt_count = 0;
    timer_init(get_timer_clock_rate() / 1000000);
}

void delay_deinit()
{
    timer_deinit();
    memset(&g_state, 0, sizeof(g_state));
}

uint64_t delay_get_timestamp()
{
    volatile uint32_t hi_count;
    volatile uint32_t lo_count;

    // The 64-bit count is created from two parts:
    // * The lower 32-bits are directly from a 32-bit hardware timer
    // * The higher bits are incremented by an interrupt whien the lower
    //      bits overflow.
    //
    // The loop ensures that a consistent combinations of the two counts
    // is used.
    do { 
        hi_count = g_state.interrupt_count;
        lo_count = DELAY_TIMER->TC;
    
    } while (hi_count != g_state.interrupt_count);

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

