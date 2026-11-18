/**
 * @file    sc7a20.h
 * @brief   Driver for the SC7A20HTR 3-axis accelerometer (Silan, LIS2DH12 class).
 *
 * The part is a near-clone of the LIS2DH12: same FIFO, same interrupt
 * sources, similar register map. Only the registers used here are listed.
 *
 * Datasheet: SC7A20H, version 0.7 (Silan, Chinese).
 *
 * Why the driver looks like this: the target's idle current is only ~30 uA
 * (see docs/manufacturing/power-budget.md) and the accelerometer is the
 * second biggest consumer of it (8.5 uA in low-power mode). The sensor
 * therefore stays in low-power mode all the time and reports *events*
 * (threshold crossings), never every sample - see docs/adr/0014.
 */

#ifndef SC7A20_H
#define SC7A20_H

#include <stdint.h>

/* ---- Registers (SC7A20H DS v0.7, chapter 12) ------------------------- */

#define SC7A20_REG_SPI_CTRL      0x0Eu /**< SPI address extension */
#define SC7A20_REG_WHO_AM_I      0x0Fu /**< reads 0x11 */
#define SC7A20_REG_CTRL0         0x1Fu /**< OSR, DLPF[1], HR */
#define SC7A20_REG_CTRL1         0x20u /**< ODR[3:0], LPen, Xen/Yen/Zen */
#define SC7A20_REG_CTRL2         0x21u /**< HDS, HPCF, FDS, HP_RESET, HPIS2/1 */
#define SC7A20_REG_CTRL3         0x22u /**< INT1 enables + FIFO_MODE */
#define SC7A20_REG_CTRL4         0x23u /**< FS[1:0], DLPF[0] */
#define SC7A20_REG_CTRL5         0x24u /**< reserved (AOI configs live at 0x30) */
#define SC7A20_REG_CTRL6         0x25u /**< reserved */
#define SC7A20_REG_STATUS        0x27u /**< DRDY status + first data byte */
#define SC7A20_REG_OUT_X_L       0x28u
#define SC7A20_REG_OUT_X_H       0x29u
#define SC7A20_REG_OUT_Y_L       0x2Au
#define SC7A20_REG_OUT_Y_H       0x2Bu
#define SC7A20_REG_OUT_Z_L       0x2Cu
#define SC7A20_REG_OUT_Z_H       0x2Du
#define SC7A20_REG_FIFO_CTRL     0x2Eu /**< FM[1:0], TR, FTH[4:0] */
#define SC7A20_REG_FIFO_SRC      0x2Fu /**< WTM, OVER, EMPTY, FSS[4:0] */
#define SC7A20_REG_AOI1_CFG      0x30u /**< INT1 event configuration */
#define SC7A20_REG_AOI1_SRC      0x31u /**< INT1 status */
#define SC7A20_REG_AOI1_THS      0x32u /**< threshold, 1 LSB = 128 mg @ +-16 g */
#define SC7A20_REG_AOI1_DUR      0x33u /**< duration counter, clocked by ODR */
#define SC7A20_REG_AOI2_CFG      0x34u
#define SC7A20_REG_AOI2_SRC      0x35u
#define SC7A20_REG_FIFO_OUT_X_L  0x61u /**< FIFO read window (auto-increment) */
#define SC7A20_REG_SOFT_RESET    0x68u
#define SC7A20_REG_FIFO_DATA     0x69u

/* CTRL0 (0x1F) */
#define SC7A20_CTRL0_HR          0x01u /**< 1 = high performance mode */
#define SC7A20_CTRL0_OSR_SHIFT   4u

/* CTRL1 (0x20) */
#define SC7A20_CTRL1_ODR_SHIFT   4u
#define SC7A20_CTRL1_LPEN        0x08u /**< 1 = low-power mode */
#define SC7A20_CTRL1_AXES        0x07u /**< Xen|Yen|Zen */

/* CTRL2 (0x21) */
#define SC7A20_CTRL2_HDS         0x40u /**< 1 = raw ADC data for the HPF */
#define SC7A20_CTRL2_HPCF_SHIFT  4u
#define SC7A20_CTRL2_FDS         0x08u
#define SC7A20_CTRL2_HP_RESET    0x04u
#define SC7A20_CTRL2_HPIS2       0x02u /**< HPF for the AOI2 source */
#define SC7A20_CTRL2_HPIS1       0x01u /**< HPF for the AOI1 source */

/* CTRL3 (0x22) - which source is routed to INT1 */
#define SC7A20_CTRL3_I1_CLICK    0x80u
#define SC7A20_CTRL3_I1_AOI1     0x40u
#define SC7A20_CTRL3_I1_AOI2     0x20u
#define SC7A20_CTRL3_I1_DRDY     0x10u
#define SC7A20_CTRL3_I1_WTM      0x04u
#define SC7A20_CTRL3_I1_OVERRUN  0x02u
#define SC7A20_CTRL3_FIFO_MODE   0x01u

/* AOI1_CFG (0x30) - INT1 event configuration */
#define SC7A20_AOI1_AOI          0x80u /**< 0 = OR event, 1 = AND event */
#define SC7A20_AOI1_6D           0x40u /**< 0 = 3D event, 1 = 6D direction */
#define SC7A20_AOI1_ZHIE         0x20u
#define SC7A20_AOI1_ZLIE         0x10u
#define SC7A20_AOI1_YHIE         0x08u
#define SC7A20_AOI1_YLIE         0x04u
#define SC7A20_AOI1_XHIE         0x02u
#define SC7A20_AOI1_XLIE         0x01u

/* CTRL4 (0x23) - full scale */
#define SC7A20_CTRL4_FS_SHIFT    4u
#define SC7A20_CTRL4_FS_2G       0x00u
#define SC7A20_CTRL4_FS_4G       0x01u
#define SC7A20_CTRL4_FS_8G       0x02u
#define SC7A20_CTRL4_FS_16G      0x03u

/* FIFO_CTRL (0x2E) */
#define SC7A20_FIFO_TRIGGER      0xC0u /**< FM = 11: triggered FIFO mode */
#define SC7A20_FIFO_STREAM       0x80u /**< FM = 10: stream mode */
#define SC7A20_FIFO_BYPASS       0x00u /**< FM = 00: FIFO off */
#define SC7A20_FIFO_TRIG_AOI2    0x20u /**< TR = 1: trigger on AOI2 */
#define SC7A20_FIFO_WTM_MASK     0x1Fu

/* FIFO_SRC (0x2F) */
#define SC7A20_FIFO_SRC_WTM      0x80u
#define SC7A20_FIFO_SRC_OVER     0x40u
#define SC7A20_FIFO_SRC_EMPTY    0x20u
#define SC7A20_FIFO_SRC_COUNT    0x1Fu

/** ODR code -> rate in Hz (CTRL1 ODR[3:0], DS table 12.4). */
#define SC7A20_ODR_1_56HZ        0x01u
#define SC7A20_ODR_12_5HZ        0x02u
#define SC7A20_ODR_25HZ          0x03u
#define SC7A20_ODR_50HZ          0x04u
#define SC7A20_ODR_100HZ         0x05u
#define SC7A20_ODR_200HZ         0x06u
#define SC7A20_ODR_400HZ         0x07u
#define SC7A20_ODR_800HZ         0x08u
#define SC7A20_ODR_1_48KHZ       0x09u /**< high performance mode only */
#define SC7A20_ODR_2_66KHZ       0x0Au
#define SC7A20_ODR_4_43KHZ       0x0Bu

/** FIFO depth of the part (samples). */
#define SC7A20_FIFO_DEPTH 16u

/** One acceleration sample, raw 12-bit register values (signed). */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} sc7a20_sample_t;

/**
 * @brief Read WHO_AM_I and check the chip id.
 * @return 0 on success.
 */
int sc7a20_probe(void);

/**
 * @brief Configure the sensor for low-power watching with a threshold
 *        interrupt on INT1 (any axis, absolute value).
 *
 * @param odr_code    ODR code (SC7A20_ODR_*).
 * @param ths_lsb     Threshold in LSBs (1 LSB = 128 mg at +-16 g).
 * @param duration    Duration counter (0 = trigger immediately).
 * @return 0 on success.
 */
int sc7a20_config_watch(uint8_t odr_code, uint8_t ths_lsb, uint8_t duration);

/**
 * @brief Switch to high-performance mode at the given ODR (capture mode).
 * @param odr_code    ODR code (1.48 kHz .. 4.43 kHz are HP only).
 * @return 0 on success.
 */
int sc7a20_config_capture(uint8_t odr_code);

/**
 * @brief Back to low-power watching mode.
 * @return 0 on success.
 */
int sc7a20_config_idle(void);

/**
 * @brief Read the FIFO samples (up to @p max samples).
 * @param out      Destination.
 * @param max      Capacity of @p out.
 * @param count    Number of samples actually read.
 * @return 0 on success.
 */
int sc7a20_read_fifo(sc7a20_sample_t *out, uint8_t max, uint8_t *count);

/**
 * @brief Number of samples currently in the FIFO.
 * @return Sample count (0..16).
 */
uint8_t sc7a20_fifo_count(void);

/**
 * @brief Read the INT1 event source status (register 0x31).
 * @return Raw status byte.
 */
uint8_t sc7a20_int1_status(void);

/**
 * @brief Convert a raw sample to g (full scale +-16 g).
 * @param raw  Raw 12-bit value.
 * @return Acceleration in g.
 */
float sc7a20_to_g(int16_t raw);

#endif /* SC7A20_H */
