/**
 * @file    app.c
 * @brief   Application layer: impact detection, packet build, transmission.
 */

#include "app.h"
#include "config.h"
#include "board.h"
#include "hal_ch32v203.h"
#include "power.h"
#include "protocol.h"
#include "sc7a20.h"

static uint32_t s_event_count;

void app_init(void)
{
    /*
     * 144 MHz gives the DSP headroom, but the idle state is entered at a
     * lower frequency: the wake-up path only needs the SPI bus and a few
     * hundred multiplications. The clock is raised again for the DSP stage
     * once the impact has been confirmed.
     */
    hal_init(HAL_CLK_PLL_144MHZ);
    power_init();
    s_event_count = 0u;
}

void app_run_once(void)
{
    power_wake_t wake = power_sleep();
    sc7a20_sample_t samples[SC7A20_FIFO_DEPTH];
    uint8_t count = 0u;

    switch (wake) {
    case POWER_WAKE_ACCEL:
        /*
         * The accelerometer reported a threshold crossing. The FIFO holds the
         * samples around the event (trigger mode).
         */
        (void)sc7a20_read_fifo(samples, SC7A20_FIFO_DEPTH, &count);
        if (count > 0u) {
            /*
             * TODO: extract the features (peak, energy, duration), classify
             * the hit, build the packet and transmit it via SX1262.
             */
            s_event_count++;
        }
        break;

    case POWER_WAKE_TIMER:
        /* Periodic wake-up: battery level and heart-beat. */
        (void)power_battery_mv();
        break;

    default:
        break;
    }
}

uint32_t app_event_count(void)
{
    return s_event_count;
}
