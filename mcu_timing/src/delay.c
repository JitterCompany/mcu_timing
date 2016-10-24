#include "delay.h"
#include "chip.h"
#include "irq.h"

static struct {
    uint32_t timer_freq_mhz;
    volatile uint32_t interrupt_count;
} g_state;

static void timer_init()
{

    // Enable timer 3 clock and reset it
    Chip_TIMER_Init(LPC_TIMER3);
    Chip_RGU_TriggerReset(RGU_TIMER3_RST);
    while (Chip_RGU_InReset(RGU_TIMER3_RST)) {}


    // Timer setup for match and interrupt at TICKRATE_HZ
    Chip_TIMER_Reset(LPC_TIMER3);
    Chip_TIMER_MatchEnableInt(LPC_TIMER3, 1);
    Chip_TIMER_SetMatch(LPC_TIMER3, 1, 0xFFFFFFFF);
    Chip_TIMER_ResetOnMatchDisable(LPC_TIMER3, 1);
    Chip_TIMER_Enable(LPC_TIMER3);

    // Enable timer3 interrupt
    NVIC_EnableIRQ(TIMER3_IRQn);
    NVIC_ClearPendingIRQ(TIMER3_IRQn);
}

void TIMER3_IRQHandler(void)
{
    if (Chip_TIMER_MatchPending(LPC_TIMER3, 1)) {
        Chip_TIMER_ClearMatch(LPC_TIMER3, 1);
        g_state.interrupt_count++;
    }
}

void delay_init()
{
    g_state.interrupt_count = 0;
    timer_init();

    g_state.timer_freq_mhz = Chip_Clock_GetRate(CLK_MX_TIMER3) / 1000000;
}

uint64_t delay_get_timestamp()
{
    // begin critical section: disable interrupts
    bool prev_irq_status = irq_disable();

    uint64_t timestamp = (((uint64_t) g_state.interrupt_count) << 32)
                         | LPC_TIMER3->TC;

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

