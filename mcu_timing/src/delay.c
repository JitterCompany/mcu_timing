#include "delay.h"
#include "chip.h"
#include <lpc_tools/irq.h>
#include <string.h>

//
// Platform specific code
//
#if (defined(MCU_PLATFORM_43xx_m4) || defined(MCU_PLATFORM_43xx_m0))
    #define DELAY_TIMER             LPC_TIMER3
    #define DELAY_TIMER_IRQn        TIMER3_IRQn
    #define DELAY_IRQHandler        TIMER3_IRQHandler

    #define DELAY_RGU_TIMER_RST     RGU_TIMER3_RST

static inline void reset_timer(void)
{
    Chip_RGU_TriggerReset(DELAY_RGU_TIMER_RST);
    while (Chip_RGU_InReset(DELAY_RGU_TIMER_RST)) {}
}
static inline uint32_t get_timer_clock_rate(void)
{
    return Chip_Clock_GetRate(CLK_MX_TIMER3);
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
    uint32_t timer_freq_mhz;
    volatile uint32_t interrupt_count;
} g_state;



static void timer_init()
{

    // Enable timer 3 clock and reset it
    Chip_TIMER_Init(DELAY_TIMER);
    reset_timer();

    // Timer setup for match and interrupt at TICKRATE_HZ
    Chip_TIMER_Reset(DELAY_TIMER);
    Chip_TIMER_MatchEnableInt(DELAY_TIMER, 1);
    Chip_TIMER_SetMatch(DELAY_TIMER, 1, 0xFFFFFFFF);
    Chip_TIMER_ResetOnMatchDisable(DELAY_TIMER, 1);
    Chip_TIMER_Enable(DELAY_TIMER);

    // Enable timer3 interrupt
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
    }
    g_state.interrupt_count++;
}

void delay_init()
{
    g_state.interrupt_count = 0;
    timer_init();

    g_state.timer_freq_mhz = get_timer_clock_rate() / 1000000;
}

void delay_deinit()
{
    timer_deinit();
    memset(&g_state, 0, sizeof(g_state));
}

uint64_t delay_get_timestamp()
{
    // begin critical section: disable interrupts
    bool prev_irq_status = irq_disable();

    uint64_t timestamp = (((uint64_t) g_state.interrupt_count) << 32)
                         | DELAY_TIMER->TC;

    // end critical section: re-enable timer interrupt
    if(prev_irq_status) {
        irq_enable();
    }

    return timestamp;
}

uint64_t delay_calc_time_us(uint64_t start_timestamp, uint64_t end_timestamp)
{
    if(start_timestamp > end_timestamp) {
        return 0;
    }

    uint64_t difference_ticks = end_timestamp - start_timestamp;
    return (difference_ticks / (uint64_t)(g_state.timer_freq_mhz));
}

void delay_us(uint64_t microseconds)
{
    delay_timeout_t timeout;
    delay_timeout_set(&timeout, microseconds);
    while(!delay_timeout_done(&timeout)) {}
}

void delay_timeout_set(delay_timeout_t *timeout, uint64_t microseconds)
{
    uint64_t current_ticks = delay_get_timestamp();
    uint64_t timeout_ticks = microseconds * g_state.timer_freq_mhz;
    timeout->target_timestamp = current_ticks + timeout_ticks;
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

