/**
 * @file    app.c
 * @brief   Application layer: impact detection, packet build, transmission.
 */

#include "app.h"
#include "config.h"
#include "board.h"
#include "hal_ch32v203.h"
#include "protocol.h"

static uint32_t s_event_count;

void app_init(void)
{
    /* 144 MHz gives the DSP headroom; 48 MHz would do for detection alone. */
    hal_init(HAL_CLK_PLL_144MHZ);
    s_event_count = 0u;
}

void app_run_once(void)
{
    /*
     * TODO: wait for the accelerometer interrupt (INT1 on PB3), read the FIFO
     * burst, extract the features, classify the hit, build the packet and
     * transmit it via SX1262.
     *
     * Until the SX1262 and SC7A20 drivers exist, the loop only advances the
     * sleep timer so the state machine can be exercised on the host.
     */
    hal_enter_deep_sleep(HAL_SLEEP_STOP, IMPACT_DEBOUNCE_MS);
}

uint32_t app_event_count(void)
{
    return s_event_count;
}
