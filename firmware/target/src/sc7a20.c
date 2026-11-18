/**
 * @file    sc7a20.c
 * @brief   SC7A20HTR driver: low-power watch mode with a threshold interrupt.
 *
 * SPI protocol (SC7A20H DS v0.7, chapter 10):
 *   - 4-wire SPI, MSB first, mode 0/3;
 *   - the first byte is RW | MS | AD5..AD0;
 *   - a read has RW = 1, the data byte comes in the second byte;
 *   - MS = 1 enables the auto-increment of the register address.
 *
 * The values below are the starting point for the bench tuning (R-03):
 * the acceleration threshold and the FIFO watermark have to be adjusted with
 * the real impact profile measured on the target.
 */

#include "sc7a20.h"

#include "board.h"
#include "hal_ch32v203.h"

#define SPI_RW_READ  0x80u
#define SPI_MSB      0x40u

/** Write one register. */
static int reg_write(uint8_t addr, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = (uint8_t)(addr & 0x3Fu);      /* RW = 0, MS = 0 */
    tx[1] = value;
    return hal_spi_transfer(HAL_SPI_1, BOARD_CS_ACC, tx, NULL, sizeof(tx));
}

/** Read one register. */
static int reg_read(uint8_t addr, uint8_t *value)
{
    uint8_t tx[2];
    uint8_t rx[2];
    tx[0] = (uint8_t)(SPI_RW_READ | (addr & 0x3Fu));
    tx[1] = 0xFFu;
    if (hal_spi_transfer(HAL_SPI_1, BOARD_CS_ACC, tx, rx, sizeof(tx)) != 0) {
        return -1;
    }
    *value = rx[1];
    return 0;
}

/** Read several registers with the auto-increment (MS = 1). */
static int reg_read_burst(uint8_t addr, uint8_t *out, uint8_t len)
{
    uint8_t tx[2];
    uint8_t rx[2];
    tx[0] = (uint8_t)(SPI_RW_READ | SPI_MSB | (addr & 0x3Fu));
    tx[1] = 0xFFu;
    /* One byte at a time keeps the stub HAL usable; the real driver should
     * use hal_spi_transfer_async with a DMA-fed buffer. */
    for (uint8_t i = 0u; i < len; i++) {
        if (hal_spi_transfer(HAL_SPI_1, BOARD_CS_ACC, tx, rx, sizeof(tx)) != 0) {
            return -1;
        }
        out[i] = rx[1];
    }
    return 0;
}

int sc7a20_probe(void)
{
    uint8_t who = 0u;
    if (reg_read(SC7A20_REG_WHO_AM_I, &who) != 0) {
        return -1;
    }
    /* DS: WHO_AM_I = 0x11. The sensor revision is checked with register 0x70,
     * which is not documented in the public DS - skipped on purpose. */
    return (who == 0x11u) ? 0 : -1;
}

int sc7a20_config_watch(uint8_t odr_code, uint8_t ths_lsb, uint8_t duration)
{
    /* CTRL0: OSR = 0 (decimated ODR), no high-performance mode. */
    if (reg_write(SC7A20_REG_CTRL0, 0x00u) != 0) {
        return -1;
    }
    /* CTRL1: low-power mode, ODR, all three axes enabled. */
    if (reg_write(SC7A20_REG_CTRL1,
                  (uint8_t)((odr_code << SC7A20_CTRL1_ODR_SHIFT) |
                            SC7A20_CTRL1_LPEN | SC7A20_CTRL1_AXES)) != 0) {
        return -1;
    }
    /* CTRL2: the AOI1 source is the high-pass filtered data, so the static
     * gravity does not occupy the threshold budget. */
    if (reg_write(SC7A20_REG_CTRL2,
                  (uint8_t)(SC7A20_CTRL2_HPIS1 | SC7A20_CTRL2_FDS)) != 0) {
        return -1;
    }
    /* CTRL3: route AOI1 (and the FIFO watermark) to INT1. The data-ready
     * interrupt stays OFF on purpose: a DRDY interrupt would wake the MCU
     * on every sample and multiply the idle current by 3-4. */
    if (reg_write(SC7A20_REG_CTRL3,
                  (uint8_t)(SC7A20_CTRL3_I1_AOI1 | SC7A20_CTRL3_I1_WTM)) != 0) {
        return -1;
    }
    /* CTRL4: full scale +-16 g (ADR-0003) and the low-pass filter off. */
    if (reg_write(SC7A20_REG_CTRL4,
                  (uint8_t)(SC7A20_CTRL4_FS_16G << SC7A20_CTRL4_FS_SHIFT)) != 0) {
        return -1;
    }
    /* AOI1_CFG: 3D event (absolute value of any axis) with the high events
     * of X, Y and Z enabled. */
    if (reg_write(SC7A20_REG_AOI1_CFG,
                  (uint8_t)(SC7A20_AOI1_XHIE | SC7A20_AOI1_YHIE |
                            SC7A20_AOI1_ZHIE)) != 0) {
        return -1;
    }
    if (reg_write(SC7A20_REG_AOI1_THS, ths_lsb) != 0) {
        return -1;
    }
    if (reg_write(SC7A20_REG_AOI1_DUR, duration) != 0) {
        return -1;
    }
    /* FIFO: trigger mode (the FIFO freezes the samples around the AOI1
     * event instead of overwriting them), watermark at 8 samples. */
    if (reg_write(SC7A20_REG_FIFO_CTRL,
                  (uint8_t)(SC7A20_FIFO_TRIGGER | 8u)) != 0) {
        return -1;
    }
    return 0;
}

int sc7a20_config_capture(uint8_t odr_code)
{
    /* High-performance mode, the same axes and the same interrupt setup;
     * only the ODR and the HR bit change. */
    if (reg_write(SC7A20_REG_CTRL0, SC7A20_CTRL0_HR) != 0) {
        return -1;
    }
    if (reg_write(SC7A20_REG_CTRL1,
                  (uint8_t)((odr_code << SC7A20_CTRL1_ODR_SHIFT) |
                            SC7A20_CTRL1_AXES)) != 0) {
        return -1;
    }
    return 0;
}

int sc7a20_config_idle(void)
{
    return sc7a20_config_watch(SC7A20_ODR_100HZ, 4u, 0u);
}

uint8_t sc7a20_fifo_count(void)
{
    uint8_t src = 0u;
    if (reg_read(SC7A20_REG_FIFO_SRC, &src) != 0) {
        return 0u;
    }
    return (uint8_t)(src & SC7A20_FIFO_SRC_COUNT);
}

uint8_t sc7a20_int1_status(void)
{
    uint8_t src = 0u;
    if (reg_read(SC7A20_REG_AOI1_SRC, &src) != 0) {
        return 0u;
    }
    return src;
}

int sc7a20_read_fifo(sc7a20_sample_t *out, uint8_t max, uint8_t *count)
{
    uint8_t n;
    uint8_t raw[6];

    if ((out == 0) || (count == 0)) {
        return -1;
    }
    n = sc7a20_fifo_count();
    if (n > max) {
        n = max;
    }
    *count = 0u;
    for (uint8_t i = 0u; i < n; i++) {
        if (reg_read_burst(SC7A20_REG_FIFO_OUT_X_L, raw, sizeof(raw)) != 0) {
            return -1;
        }
        out[i].x = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
        out[i].y = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
        out[i].z = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);
    }
    *count = n;
    return 0;
}

float sc7a20_to_g(int16_t raw)
{
    /* +-16 g full scale over 12 bit: 1 g = 1024 LSB (DS table 12.11). */
    return (float)raw / 1024.0f;
}
