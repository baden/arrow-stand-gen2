/**
 * @file    power.h
 * @brief   Power budget constants and the sleep state machine of the target.
 *
 * The numbers below are the design budget from
 * docs/manufacturing/power-budget.md (v2.0, 2026-11-18). They are the
 * acceptance criteria for the bench measurement R-16:
 *
 *   idle (sleeping)   <= 30 uA   (measured 493 uA before ADR-0014)
 *   wake latency      ~300 us    (CH32V203 Stop mode, regulator low-power)
 *   TX burst          32 mA      (+14 dBm, 811 ms on air)
 *
 * The rules the firmware must respect to keep the budget:
 *
 *  1. The MCU sleeps in **Stop mode** with the internal regulator in
 *     **low-power mode** (10.5 uA instead of 70.5 uA).
 *  2. The accelerometer is woken up **by events, not by samples**: the DRDY
 *     interrupt stays disabled, the AOI1 threshold interrupt on INT1 is the
 *     only wake source from the sensor. Waking on every sample at 100 Hz
 *     would add ~100 uA and multiply the whole budget by 3-4.
 *  3. PA4 (BUSY) and PA8 (DIO1) are **floating inputs without pull** while
 *     sleeping: the SX1262 pulls them low with internal 50 kOhm pull-downs
 *     in SLEEP mode, and an MCU pull-up against that would burn ~82 uA.
 *  4. The battery divider is **switched off** (Q1) except during a
 *     measurement.
 *  5. The radio is never reset after the initialisation (warm start): a
 *     cold start would cost ~5 mC per event, a third of the TX energy.
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>

/* ---- Budget (docs/manufacturing/power-budget.md) ---------------------- */

/** Idle current of the whole target, microamps (acceptance criterion). */
#define POWER_IDLE_TARGET_UA 30u

/** Idle current before ADR-0014 (for comparison in the logs). */
#define POWER_IDLE_BEFORE_UA 493u

/** Budget of a single block in the sleep state, microamps. */
#define POWER_IDLE_MCU_UA 10u   /**< CH32V203 Stop, regulator low-power */
#define POWER_IDLE_ACC_UA 8u    /**< SC7A20 low-power mode @100 Hz */
#define POWER_IDLE_LDO_UA 8u    /**< XC6206P332MR Iq */
#define POWER_IDLE_RADIO_UA 1u  /**< SX1262 SLEEP, warm start */
#define POWER_IDLE_CHG_UA 2u    /**< TP4056 reverse leakage, VCC off */

/** Wake-up latency of the MCU from Stop mode, microseconds (DS Table 4-8). */
#define POWER_WAKE_LATENCY_US 300u

/** Transmit current of the radio at +14 dBm, milliamperes (DS Table 3-6). */
#define POWER_TX_CURRENT_MA 32u

/** Time on air of one event packet at SF11/BW125 kHz, milliseconds. */
#define POWER_PACKET_TOA_MS 811u

/** Energy of one transmitted event, milliampere-hours. */
#define POWER_EVENT_MAH 0.026f

/* ---- Behaviour -------------------------------------------------------- */

/** Periodic wake-up (battery measurement, heart-beat), milliseconds. */
#define POWER_PERIODIC_WAKE_MS 60000u

/** Divider ratio of the battery sense (R2 : R3 = 22k : 22k). */
#define POWER_BATT_DIVIDER_RATIO 2u

/** Settling time of the battery divider after Q1 is switched on, ms.
 *  tau = (R2||R3) x C11 = 11 kOhm x 10 nF = 110 us; 5 tau + margin. */
#define POWER_BATT_SETTLE_MS 2u

/** Reason why the MCU woke up. */
typedef enum {
    POWER_WAKE_UNKNOWN = 0,
    POWER_WAKE_ACCEL,   /**< the accelerometer reported an event (INT1) */
    POWER_WAKE_TIMER    /**< the periodic timer expired */
} power_wake_t;

/**
 * @brief Prepare the board for the sleep state machine.
 *
 * Configures the accelerometer for low-power watching with a threshold
 * interrupt, enables the EXTI line on INT1, puts the radio into SLEEP with
 * the configuration retained, and switches the battery divider off.
 */
void power_init(void);

/**
 * @brief Sleep until an event or the periodic timer expires.
 *
 * @return The wake-up reason.
 */
power_wake_t power_sleep(void);

/**
 * @brief Measure the battery voltage (switches the divider on and off).
 * @return Battery voltage in millivolts, 0 on failure.
 */
uint16_t power_battery_mv(void);

#endif /* POWER_H */
