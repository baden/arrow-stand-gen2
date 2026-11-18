/**
 * @file    board.h
 * @brief   Pin assignment of the arrow-stand-gen2 HUB board (базова станція).
 *
 * Verified against the schematic (hardware/hub/arrow-stand-hub.kicad_sch) and
 * the CH32V203 datasheet (QFN-48 pinout, ADR-0011).
 *
 * MCU: CH32V203C8U6 (QFN-48 7x7), 49 pins (EP = 49).
 * The hub receives LoRa packets, latches 10 channel LEDs for 3 s each and
 * streams the events to the PC over USB CDC (ADR-0012).
 */

#ifndef BOARD_H
#define BOARD_H

#include "hal_ch32v203.h"

/* ---- SPI1 (fixed by the peripheral on CH32V203) ---------------------- */

#define BOARD_SPI_SCK   { HAL_PORT_A, 5 }  /**< pin 15 */
#define BOARD_SPI_MOSI  { HAL_PORT_A, 7 }  /**< pin 17 */
#define BOARD_SPI_MISO  { HAL_PORT_A, 6 }  /**< pin 16 */

/* ---- Radio (SX1262, RX only) ----------------------------------------- */

#define BOARD_CS_RADIO     { HAL_PORT_A, 2 }  /**< pin 12 -> U1 NSS (19) */
#define BOARD_RADIO_BUSY   { HAL_PORT_A, 4 }  /**< pin 14, input -> BUSY (14) */
#define BOARD_RADIO_DIO1   { HAL_PORT_A, 8 }  /**< pin 29, input -> DIO1 (13) */
#define BOARD_RADIO_NRST   { HAL_PORT_A, 9 }  /**< pin 30, output -> NRESET (15) */

/* ---- USB FS (CDC -> PC) ----------------------------------------------- */

#define BOARD_USB_DM       { HAL_PORT_A, 11 } /**< pin 32, PA11/USB1DM */
#define BOARD_USB_DP       { HAL_PORT_A, 12 } /**< pin 33, PA12/USB1DP */

/* ---- Programming (2-wire WCH ISP) ------------------------------------ */

#define BOARD_ISP_PDI      { HAL_PORT_A, 13 } /**< pin 34, SWDIO -> TP1 */
#define BOARD_ISP_SCLK     { HAL_PORT_A, 14 } /**< pin 37, SWCLK -> TP2 */

/* ---- User input -------------------------------------------------------- */

#define BOARD_BTN          { HAL_PORT_A, 0 }  /**< pin 10, input, pull-up (clear channels) */

/* ---- Channel indicators ------------------------------------------------- */

/**
 * Ten channel LEDs. Each output drives an LED through a 1 kOhm resistor; the
 * LED is latched for 3 s after a hit (ADR-0012).
 */
#define BOARD_LED1  { HAL_PORT_B, 3 }   /**< pin 39 */
#define BOARD_LED2  { HAL_PORT_B, 4 }   /**< pin 40 */
#define BOARD_LED3  { HAL_PORT_B, 5 }   /**< pin 41 */
#define BOARD_LED4  { HAL_PORT_B, 8 }   /**< pin 45 */
#define BOARD_LED5  { HAL_PORT_B, 9 }   /**< pin 46 */
#define BOARD_LED6  { HAL_PORT_B, 10 }  /**< pin 21 */
#define BOARD_LED7  { HAL_PORT_B, 11 }  /**< pin 22 */
#define BOARD_LED8  { HAL_PORT_B, 12 }  /**< pin 25 */
#define BOARD_LED9  { HAL_PORT_B, 13 }  /**< pin 26 */
#define BOARD_LED10 { HAL_PORT_B, 14 }  /**< pin 27 */

/** Status LED ("radio alive"). */
#define BOARD_LED_STATUS { HAL_PORT_B, 15 } /**< pin 28 */

/* ---- Spare -------------------------------------------------------------- */

/** Free GPIOs: PA1 (ADC1), PA3 (ADC3), PB0 (ADC8), PB1 (ADC9), PA10, PA15, PB6, PB7. */
#define BOARD_GPIO_SPARE1 { HAL_PORT_A, 1 }
#define BOARD_GPIO_SPARE2 { HAL_PORT_A, 3 }
#define BOARD_GPIO_SPARE3 { HAL_PORT_B, 0 }
#define BOARD_GPIO_SPARE4 { HAL_PORT_B, 1 }
#define BOARD_GPIO_SPARE5 { HAL_PORT_A, 10 }
#define BOARD_GPIO_SPARE6 { HAL_PORT_A, 15 }

/**
 * PB2 is the BOOT1 strap of the QFN-48 package and is tied to GND
 * (guaranteed "boot from flash"), so it is NOT usable as a GPIO.
 * NRST of the MCU has no external reset network: WCH-Link resets over DAP.
 */

#endif /* BOARD_H */
