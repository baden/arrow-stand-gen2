/**
 * @file    hal_ch32v203.h
 * @brief   HAL for the WCH CH32V203F8U6 MCU (RISC-V QingKe V4B, 144 MHz).
 *
 * CH32V203F8U6: 64 KB Flash (zero-wait area), 20 KB SRAM, internal 8 MHz RC
 * + PLL up to 144 MHz, 2x SPI (one used here), 12-bit ADC, 18-channel DMA,
 * USB full-speed device, CAN, hardware multiply/divide.
 *
 * The register-level implementation depends on the WCH MounRiver SDK headers
 * (ch32v20x.h). Until the SDK is wired into the build, the functions below
 * are declared and the .c file provides stubs, so the application layer can
 * be written and host-tested first.
 *
 * Board-level pin assignment lives in board.h, not here.
 */

#ifndef HAL_CH32V203_H
#define HAL_CH32V203_H

#include <stdint.h>
#include <stddef.h>

/** GPIO port. */
typedef enum {
    HAL_PORT_A = 0,
    HAL_PORT_B,
    HAL_PORT_C,
    HAL_PORT_D
} hal_gpio_port_t;

/** GPIO pin, addressed as (port, pin). */
typedef struct {
    hal_gpio_port_t port;
    uint8_t pin; /**< 0..15 */
} hal_gpio_t;

/** SPI bus index. SPI1 is the one used on this board. */
typedef enum {
    HAL_SPI_1 = 0
} hal_spi_bus_t;

/** Clock source. */
typedef enum {
    HAL_CLK_HSI_8MHZ = 0, /**< internal 8 MHz RC, no crystal */
    HAL_CLK_PLL_48MHZ,     /**< internal RC + PLL */
    HAL_CLK_PLL_144MHZ     /**< internal RC + PLL, max core clock */
} hal_clk_src_t;

/** Pin direction / configuration. */
typedef enum {
    HAL_GPIO_INPUT = 0,
    HAL_GPIO_INPUT_PULLUP,
    HAL_GPIO_INPUT_PULLDOWN,
    HAL_GPIO_OUTPUT,
    HAL_GPIO_OUTPUT_OD,
    HAL_GPIO_AF_PP,   /**< alternate function, push-pull (SPI, USART) */
    HAL_GPIO_AF_OD    /**< alternate function, open-drain (I2C) */
} hal_gpio_mode_t;

/** Low-power mode. */
typedef enum {
    HAL_SLEEP_STOP = 0, /**< stop mode: peripherals off, RAM retained */
    HAL_SLEEP_STANDBY   /**< standby mode: RAM mostly retained */
} hal_sleep_mode_t;

/**
 * @brief  Initialise clocks, GPIO and the SPI peripheral.
 * @param  clk Clock source.
 * @return 0 on success.
 */
int hal_init(hal_clk_src_t clk);

/**
 * @brief  Milliseconds since boot (wraps every ~49 days).
 */
uint32_t hal_millis(void);

/**
 * @brief  Busy wait.
 * @param  ms Milliseconds to wait.
 */
void hal_delay_ms(uint32_t ms);

/**
 * @brief  Configure a pin.
 * @param  pin  Pin.
 * @param  mode Mode to apply.
 */
void hal_gpio_mode(hal_gpio_t pin, hal_gpio_mode_t mode);

/**
 * @brief  Read a pin.
 * @param  pin Pin.
 * @return 0 or 1.
 */
uint8_t hal_gpio_read(hal_gpio_t pin);

/**
 * @brief  Write a pin.
 * @param  pin   Pin.
 * @param  value 0 or 1.
 */
void hal_gpio_write(hal_gpio_t pin, uint8_t value);

/**
 * @brief  SPI transfer with automatic CS handling.
 *
 * The board shares one SPI bus between the radio and the accelerometer, so
 * the chip-select is an argument here rather than a fixed pin.
 *
 * @param  bus  SPI bus.
 * @param  cs   Chip-select pin (driven low for the transfer).
 * @param  tx   Bytes to send (may be NULL).
 * @param  rx   Buffer for received bytes (may be NULL).
 * @param  len  Number of bytes.
 * @return 0 on success.
 */
int hal_spi_transfer(hal_spi_bus_t bus, hal_gpio_t cs,
                     const uint8_t *tx, uint8_t *rx, size_t len);

/**
 * @brief  Start an SPI transfer without waiting (DMA driven).
 * @param  bus  SPI bus.
 * @param  cs   Chip-select pin.
 * @param  tx   Bytes to send.
 * @param  rx   Buffer for received bytes.
 * @param  len  Number of bytes.
 * @return 0 on success.
 */
int hal_spi_transfer_async(hal_spi_bus_t bus, hal_gpio_t cs,
                           const uint8_t *tx, uint8_t *rx, size_t len);

/**
 * @brief  Wait for an asynchronous SPI transfer to finish.
 * @return 0 on success.
 */
int hal_spi_wait(void);

/**
 * @brief  Read the battery voltage through the ADC divider.
 * @param  divider_ratio R_upper / R_lower of the sense divider.
 * @return Millivolts, or 0 if not implemented yet.
 */
uint16_t hal_battery_mv(uint32_t divider_ratio);

/**
 * @brief  Enable an external interrupt on a pin (EXTI).
 * @param  pin    Pin.
 * @param  rising Trigger on the rising edge.
 * @param  falling Trigger on the falling edge.
 */
void hal_exti_enable(hal_gpio_t pin, uint8_t rising, uint8_t falling);

/**
 * @brief  Enter a low-power mode until an interrupt or a wake timer.
 * @param  mode      Low-power mode.
 * @param  wake_ms   Wake-up timer, milliseconds. 0 = wait for interrupt only.
 */
void hal_enter_deep_sleep(hal_sleep_mode_t mode, uint32_t wake_ms);

/**
 * @brief  Enable or disable the USB full-speed device (hub variant only).
 * @param  enable Non-zero to enable.
 * @return 0 on success.
 */
int hal_usb_enable(uint8_t enable);

/**
 * @brief  Send a block over USB CDC (hub variant only).
 * @param  data Bytes to send.
 * @param  len  Number of bytes.
 * @return Number of bytes sent.
 */
size_t hal_usb_write(const uint8_t *data, size_t len);

#endif /* HAL_CH32V203_H */
