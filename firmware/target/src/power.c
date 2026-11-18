/**
 * @file    power.c
 * @brief   Sleep state machine of the target (see power.h for the budget).
 */

#include "power.h"

#include "board.h"
#include "config.h"
#include "hal_ch32v203.h"
#include "sc7a20.h"

/** AOI1_SRC bit 6: an interrupt event is latched. */
#define SC7A20_AOI1_SRC_IA 0x40u

void power_init(void)
{
    /*
     * The radio and the accelerometer are initialised by their own drivers;
     * here we only make sure that the board is in the low-power state.
     */

    /* Battery divider off: 0 uA in sleep instead of 18.5 uA. */
    hal_gpio_mode(BOARD_BATT_DIV_EN, HAL_GPIO_OUTPUT);
    hal_gpio_write(BOARD_BATT_DIV_EN, 0u);

    /*
     * BUSY and DIO1 are pulled low by the SX1262 itself while it sleeps
     * (internal 50 kOhm pull-downs, DS Table 8-3). They must be floating
     * inputs here, otherwise the MCU pull-up would fight them and burn
     * ~82 uA permanently.
     */
    hal_gpio_mode(BOARD_RADIO_BUSY, HAL_GPIO_INPUT);
    hal_gpio_mode(BOARD_RADIO_DIO1, HAL_GPIO_INPUT);

    /* INT1 from the accelerometer: the only wake source from the sensor. */
    hal_gpio_mode(BOARD_ACC_INT1, HAL_GPIO_INPUT);
    hal_exti_enable(BOARD_ACC_INT1, 1u, 0u);

    /* Low-power watching: 100 Hz, threshold 0.5 g, FIFO in trigger mode. */
    (void)sc7a20_config_watch(SC7A20_ODR_100HZ, ACCEL_AOI_THRESHOLD_LSB, 0u);
}

power_wake_t power_sleep(void)
{
    power_wake_t reason = POWER_WAKE_UNKNOWN;
    uint8_t src;

    /*
     * Low-power regulator before Stop mode: 10.5 uA instead of 70.5 uA
     * (DS V3.0, Table 4-8-1). Wake-up takes ~300 us.
     */
    hal_set_regulator_low_power(1u);
    hal_enter_deep_sleep(HAL_SLEEP_STOP, POWER_PERIODIC_WAKE_MS);
    hal_set_regulator_low_power(0u);

    /* Which source woke us up? The accelerometer latches the event in
     * AOI1_SRC, the pin itself returns to the idle level. */
    src = sc7a20_int1_status();
    if ((src & SC7A20_AOI1_SRC_IA) != 0u) {
        reason = POWER_WAKE_ACCEL;
    } else {
        reason = POWER_WAKE_TIMER;
    }
    return reason;
}

uint16_t power_battery_mv(void)
{
    uint16_t mv;

    hal_gpio_write(BOARD_BATT_DIV_EN, 1u);   /* Q1 on */
    hal_delay_ms(POWER_BATT_SETTLE_MS);      /* RC settle */
    mv = hal_battery_mv(POWER_BATT_DIVIDER_RATIO);
    hal_gpio_write(BOARD_BATT_DIV_EN, 0u);   /* Q1 off */

    return mv;
}
