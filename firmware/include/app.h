/**
 * @file    app.h
 * @brief   Application layer of the arrow-stand-gen2 firmware.
 */

#ifndef APP_H
#define APP_H

#include <stdint.h>

/**
 * @brief  Initialise peripherals and the application state machine.
 */
void app_init(void);

/**
 * @brief  One iteration of the main loop.
 *
 * Wakes on an accelerometer interrupt, builds the event packet and transmits
 * it, then returns to deep sleep.
 */
void app_run_once(void);

/**
 * @brief  Number of impact events detected since boot.
 */
uint32_t app_event_count(void);

#endif /* APP_H */
