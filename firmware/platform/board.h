/**
 * @file    board.h
 * @brief   Pin assignment of the arrow-stand-gen2 target board.
 *
 * Single source of truth for the firmware's view of the board. Verified
 * against the schematic (hardware/kicad/arrow-stand-gen2.kicad_sch) and the
 * CH32V203 datasheet V2.7 p.18 (QFN-20 pinout).
 *
 * MCU: CH32V203F8U6 (QFN-20 3x3), 21 pins (0 = EP/GND).
 * One SPI bus for the radio and the accelerometer, separate chip-selects
 * (the SX1262 needs its NSS held while BUSY is polled).
 */

#ifndef BOARD_H
#define BOARD_H

#include "hal_ch32v203.h"

/* ---- SPI1 (fixed by the peripheral on CH32V203) ---------------------- */

#define BOARD_SPI_SCK   { HAL_PORT_A, 5 }  /**< pin 6 */
#define BOARD_SPI_MOSI  { HAL_PORT_A, 7 }  /**< pin 7 */
#define BOARD_SPI_MISO  { HAL_PORT_A, 6 }  /**< pin 20 */

/* ---- Chip selects --------------------------------------------------- */

#define BOARD_CS_RADIO  { HAL_PORT_A, 2 }  /**< pin 3 -> SX1262 NSS (19) */
#define BOARD_CS_ACC    { HAL_PORT_A, 3 }  /**< pin 4 -> SC7A20 CS (10) */

/* ---- Radio (SX1262) ------------------------------------------------- */

#define BOARD_RADIO_BUSY { HAL_PORT_A, 4 } /**< pin 5, input -> BUSY (14) */
#define BOARD_RADIO_DIO1 { HAL_PORT_A, 8 } /**< pin 14, input -> DIO1 (13) */
#define BOARD_RADIO_NRST { HAL_PORT_A, 9 } /**< pin 15, output -> NRESET (15) */

/* ---- Accelerometer (SC7A20) ------------------------------------------ */

#define BOARD_ACC_INT1   { HAL_PORT_B, 0 } /**< pin 8, input, EXTI -> INT1 (5) */

/* ---- User input / analogue ------------------------------------------- */

#define BOARD_BTN        { HAL_PORT_A, 0 } /**< pin 1, input, pull-up, wake-up */
#define BOARD_ADC_BATT   { HAL_PORT_A, 1 } /**< pin 2 -> VBAT divider -> ADC1 */

/* ---- Programming (2-wire WCH ISP) ------------------------------------ */

#define BOARD_ISP_PDI    { HAL_PORT_A, 13 } /**< pin 16 -> TP1 */
#define BOARD_ISP_SCLK   { HAL_PORT_A, 14 } /**< pin 17 -> TP2 */

/* ---- Spare / reserved ------------------------------------------------ */

/**
 * Reserved for the piezo shock channel (plan B, ADR-0003): the piezo plus a
 * comparator gives a digital "hit" line, so no extra ADC channel is needed.
 */
#define BOARD_PIEZO      { HAL_PORT_B, 10 } /**< pin 10, NC in the schematic */

/** Other free GPIOs (marked NC in the schematic): PB1, PB11, PB14, PB15, PA10. */
#define BOARD_GPIO_SPARE1 { HAL_PORT_B, 1 }
#define BOARD_GPIO_SPARE2 { HAL_PORT_B, 11 }
#define BOARD_GPIO_SPARE3 { HAL_PORT_B, 14 }
#define BOARD_GPIO_SPARE4 { HAL_PORT_B, 15 }
#define BOARD_GPIO_SPARE5 { HAL_PORT_A, 10 }

/**
 * No free ADC channel on this package: PA0..PA7 are all in use
 * (BTN, VBAT_ADC, CS_RADIO, CS_ACC, BUSY, SCK, MISO, MOSI).
 * A second analogue input would require re-mapping a pin.
 */

#endif /* BOARD_H */
