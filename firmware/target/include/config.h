/**
 * @file    config.h
 * @brief   Build-time configuration for the arrow-stand-gen2 firmware.
 *
 * Board:  CH32V203C8U6 + SX1262IMLTRT + SC7A20HTR
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

/**
 * Default spreading factor.
 *
 * ADR-0015: SF must become a **runtime parameter** (1 byte in the packet),
 * so that the "economy" SF9 and the "insurance" SF11/SF12 can be switched
 * without reflashing. Rationale and measurements:
 * docs/research/radio-alternatives.md.
 *
 * SF11 buys +6 dB sensitivity vs SF9, but the link already has 76 dB of
 * margin at 1 km, so the extra margin is bought for 3.5x the airtime
 * (0.643 s vs 0.181 s) and 3.5x the TX energy per event.
 */
#define LORA_SF 11u

/** Minimum SF the firmware accepts from the hub's config packet. */
#define LORA_SF_MIN 7u

/** Maximum SF the firmware accepts from the hub's config packet.
 *  LLCC68 supports up to SF9 at 125 kHz, SX1262/SX1268 up to SF12. */
#define LORA_SF_MAX 12u

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

/**
 * Output data rate of the accelerometer while **watching** (Hz).
 *
 * The sensor stays in low-power mode at 100 Hz all the time (8.5 uA) and
 * reports only threshold crossings - see docs/adr/0014. Waking the MCU on
 * every sample would add ~100 uA.
 */
#define ACCEL_ODR_HZ 100u

/**
 * Output data rate used while logging an impact (R-03), high-performance
 * mode: 1.48 kHz gives ~11 ms of window in the 16-sample FIFO.
 */
#define ACCEL_ODR_HZ_CAPTURE 1480u

/**
 * Threshold of the AOI1 interrupt (the only wake source from the sensor),
 * in LSBs of the threshold register. 1 LSB = 128 mg at the +-16 g range,
 * so 4 LSB = 0.5 g. The sensor compares the high-pass filtered data, so the
 * static gravity does not eat the threshold budget.
 */
#define ACCEL_AOI_THRESHOLD_LSB 4u

/** Minimum time between two reported events, milliseconds. */
#define IMPACT_DEBOUNCE_MS 250u

/* ---- Power ------------------------------------------------------------ */

/** Deep sleep current target, microamps (docs/manufacturing/power-budget.md). */
#define POWER_SLEEP_TARGET_UA 30u

/* ---- Packet format ---------------------------------------------------- */

#define PACKET_MAGIC 0xA5u
#define PACKET_SIZE_BYTES 17u

#endif /* CONFIG_H */
