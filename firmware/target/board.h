/**
 * @file    board.h
 * @brief   Pin assignment of the arrow-stand-gen2 TARGET board (мішень).
 *
 * Single source of truth for the firmware's view of the board. Verified
 * against the schematic (hardware/target/arrow-stand-gen2.kicad_sch) and the
 * CH32V203 datasheet (QFN-48 pinout, ADR-0011).
 *
 * MCU: CH32V203C8U6 (QFN-48 7x7), 49 pins (EP = 49).
 * One SPI bus for the radio and the accelerometer, separate chip-selects
 * (the SX1262 needs its NSS held while BUSY is polled).
 * USB FS (CDC) is present: PA11 = DM, PA12 = DP.
 */

#ifndef BOARD_H
#define BOARD_H

#include "hal_ch32v203.h"

/* ---- SPI1 (fixed by the peripheral on CH32V203) ---------------------- */

#define BOARD_SPI_SCK   { HAL_PORT_A, 5 }  /**< pin 15 */
#define BOARD_SPI_MOSI  { HAL_PORT_A, 7 }  /**< pin 17 */
#define BOARD_SPI_MISO  { HAL_PORT_A, 6 }  /**< pin 16 */

/* ---- Chip selects --------------------------------------------------- */

#define BOARD_CS_RADIO  { HAL_PORT_A, 2 }  /**< pin 12 -> SX1262 NSS (19) */
#define BOARD_CS_ACC    { HAL_PORT_A, 3 }  /**< pin 13 -> SC7A20 CS (10) */

/* ---- Radio (SX1262) ------------------------------------------------- */

#define BOARD_RADIO_BUSY { HAL_PORT_A, 4 } /**< pin 14, input -> BUSY (14) */
#define BOARD_RADIO_DIO1 { HAL_PORT_A, 8 } /**< pin 29, input -> DIO1 (13) */
#define BOARD_RADIO_NRST { HAL_PORT_A, 9 } /**< pin 30, output -> NRESET (15) */

/* ---- Accelerometer (SC7A20) ------------------------------------------ */

#define BOARD_ACC_INT1   { HAL_PORT_B, 0 } /**< pin 18, input, EXTI -> INT1 (5) */

/* ---- User input / analogue ------------------------------------------- */

#define BOARD_BTN        { HAL_PORT_A, 0 } /**< pin 10, input, pull-up, wake-up */
#define BOARD_ADC_BATT   { HAL_PORT_A, 1 } /**< pin 11 -> VBAT divider -> ADC1 */

/* ---- USB FS (CDC) ---------------------------------------------------- */

#define BOARD_USB_DM     { HAL_PORT_A, 11 } /**< pin 32, PA11/USB1DM */
#define BOARD_USB_DP     { HAL_PORT_A, 12 } /**< pin 33, PA12/USB1DP */

/* ---- Programming (2-wire WCH ISP) ------------------------------------ */

#define BOARD_ISP_PDI    { HAL_PORT_A, 13 } /**< pin 34, SWDIO -> TP1 */
#define BOARD_ISP_SCLK   { HAL_PORT_A, 14 } /**< pin 37, SWCLK -> TP2 */

/* ---- Spare / reserved ------------------------------------------------ */

/**
 * Reserved for the piezo shock channel (plan B, ADR-0003): the piezo plus a
 * comparator gives a digital "hit" line, so no extra ADC channel is needed.
 */
#define BOARD_PIEZO      { HAL_PORT_B, 10 } /**< pin 21 */

/** The only free ADC channel on this package: PB1 = ADC9. */
#define BOARD_ADC_SPARE  { HAL_PORT_B, 1 }  /**< pin 19 */

/** Other free GPIOs: PB11, PB12, PB13, PB14, PB15, PA10, PA15, PB3…PB9. */
#define BOARD_GPIO_SPARE1 { HAL_PORT_B, 11 }
#define BOARD_GPIO_SPARE2 { HAL_PORT_B, 12 }
#define BOARD_GPIO_SPARE3 { HAL_PORT_B, 13 }
#define BOARD_GPIO_SPARE4 { HAL_PORT_B, 14 }
#define BOARD_GPIO_SPARE5 { HAL_PORT_B, 15 }
#define BOARD_GPIO_SPARE6 { HAL_PORT_A, 10 }
#define BOARD_GPIO_SPARE7 { HAL_PORT_A, 15 }

/**
 * PB2 is the BOOT1 strap of the QFN-48 package and is tied to GND
 * (guaranteed "boot from flash"), so it is NOT usable as a GPIO.
 * NRST of the MCU has no external reset network: WCH-Link resets over DAP.
 */

#endif /* BOARD_H */
