/**
 * @file    hal_ch32v203.c
 * @brief   HAL stubs for the WCH CH32V203F8U6.
 *
 * TODO: replace the stubs with real register access once the WCH MounRiver
 * SDK (ch32v20x.h) is added to the include path. Until then the application
 * layer can be compiled and host-tested without hardware.
 */

#include "hal_ch32v203.h"

static uint32_t s_millis;

int hal_init(hal_clk_src_t clk)
{
    (void)clk;
    s_millis = 0u;
    return 0;
}

uint32_t hal_millis(void)
{
    return s_millis;
}

void hal_delay_ms(uint32_t ms)
{
    s_millis += ms;
}

void hal_gpio_mode(hal_gpio_t pin, hal_gpio_mode_t mode)
{
    (void)pin;
    (void)mode;
}

uint8_t hal_gpio_read(hal_gpio_t pin)
{
    (void)pin;
    return 0u;
}

void hal_gpio_write(hal_gpio_t pin, uint8_t value)
{
    (void)pin;
    (void)value;
}

int hal_spi_transfer(hal_spi_bus_t bus, hal_gpio_t cs,
                     const uint8_t *tx, uint8_t *rx, size_t len)
{
    (void)bus;
    (void)cs;
    (void)tx;
    (void)rx;
    (void)len;
    return -1; /* not implemented */
}

int hal_spi_transfer_async(hal_spi_bus_t bus, hal_gpio_t cs,
                           const uint8_t *tx, uint8_t *rx, size_t len)
{
    (void)bus;
    (void)cs;
    (void)tx;
    (void)rx;
    (void)len;
    return -1; /* not implemented */
}

int hal_spi_wait(void)
{
    return -1; /* not implemented */
}

uint16_t hal_battery_mv(uint32_t divider_ratio)
{
    (void)divider_ratio;
    return 0u;
}

void hal_exti_enable(hal_gpio_t pin, uint8_t rising, uint8_t falling)
{
    (void)pin;
    (void)rising;
    (void)falling;
}

void hal_enter_deep_sleep(hal_sleep_mode_t mode, uint32_t wake_ms)
{
    (void)mode;
    (void)wake_ms;
    s_millis += 1u;
}

void hal_set_regulator_low_power(uint8_t enable)
{
    /*
     * TODO(SDK): the real implementation needs the MounRiver header
     * (ch32v20x.h). The semantics are fixed by the datasheet:
     *   PWR->CTLR  - select the regulator mode for Stop mode
     *                (1 = low-power mode -> 10.5 uA, 0 = run mode -> 70.5 uA)
     * The exact bit position depends on the SDK header, hence the TODO.
     */
    (void)enable;
}

int hal_usb_enable(uint8_t enable)
{
    (void)enable;
    return -1; /* not implemented */
}

size_t hal_usb_write(const uint8_t *data, size_t len)
{
    (void)data;
    return 0u;
}
