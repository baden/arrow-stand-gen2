/**
 * @file    config.h
 * @brief   Build-time configuration for the arrow-stand-gen2 firmware.
 *
 * Board:  CH32V203F8U6 + SX1262IMLTRT + SC7A20HTR
 * Target: LoRa impact sensor for a shooting target.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Identity ------------------------------------------------------- */

/** Protocol version carried in the event packet. */
#define PACKET_VERSION 0x02u

/* ---- LoRa parameters (ADR-0005, ADR-0007) --------------------------- */

/** Carrier frequency in Hz. 433 MHz (ADR-0005: forest, hilly terrain). */
#define LORA_FREQ_HZ 433000000uL

/** Spreading factor. 11 buys +6 dB sensitivity => x2 range for >1 km. */
#define LORA_SF 11u

/** Bandwidth in Hz. */
#define LORA_BW_HZ 125000uL

/** Coding rate denominator (4/5 .. 4/8). */
#define LORA_CR 5u

/** Preamble length in symbols. */
#define LORA_PREAMBLE 12u

/** Number of transmission attempts per event. */
#define LORA_TX_RETRIES 3u

/** Delay between retries, milliseconds. */
#define LORA_RETRY_DELAY_MS 250u

/* ---- Impact detection ------------------------------------------------ */

/** Wake-up threshold in g (sensor full scale is +-16 g). */
#define IMPACT_THRESHOLD_G 4.0f

/** Output data rate of the accelerometer in Hz. */
#define ACCEL_ODR_HZ 1600u

/**
 * Output data rate used while logging an impact (R-03): 3.2 kHz gives
 * ~10 ms of window in the 32-sample FIFO and does not miss the peak.
 */
#define ACCEL_ODR_HZ_CAPTURE 3200u

/** Minimum time between two reported events, milliseconds. */
#define IMPACT_DEBOUNCE_MS 250u

/* ---- Power ------------------------------------------------------------ */

/** Deep sleep current target, microamps. */
#define POWER_SLEEP_TARGET_UA 5u

/* ---- Packet format ---------------------------------------------------- */

#define PACKET_MAGIC 0xA5u
#define PACKET_SIZE_BYTES 17u

#endif /* CONFIG_H */
