/**
 * @file    protocol.h
 * @brief   Event packet format and CRC (host-testable, no hardware deps).
 *
 * Wire format v2, 17 bytes, little endian:
 *
 *   off  size  field
 *   0    1    magic 0xA5
 *   1    1    version (0x02)
 *   2    2    device_id
 *   4    4    seq (event counter)
 *   8    2    peak_mg (peak vector magnitude, milli-g)
 *   10   2    impulse (integral of |a| over the window, mg*s)
 *   12   1    hit_class (see hit_class_t)
 *   13   1    flags (see evt_flags_t)
 *   14   2    batt_mv (battery millivolts)
 *   16   1    crc8 over bytes 0..15
 *
 * All classification happens on the device (ADR-0008); the hub only receives
 * the result plus the features needed to re-classify if needed.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/** Serialised event packet. */
typedef struct {
    uint8_t  magic;     /**< PACKET_MAGIC */
    uint8_t  version;   /**< PACKET_VERSION */
    uint16_t device_id; /**< unique device id */
    uint32_t seq;       /**< event counter */
    uint16_t peak_mg;   /**< peak vector magnitude, milli-g */
    uint16_t impulse;   /**< integral of |a| over the window, mg*s */
    uint8_t  hit_class; /**< hit_class_t */
    uint8_t  flags;     /**< evt_flags_t */
    uint16_t batt_mv;   /**< battery voltage, millivolts */
    uint8_t  crc;       /**< CRC-8 over the preceding bytes */
} evt_packet_t;

/** Event flags. */
typedef enum {
    EVENT_FLAG_SATURATED = 1u << 0, /**< signal clipped at the sensor range */
    EVENT_FLAG_LOW_BATT = 1u << 1,  /**< battery below the warning threshold */
    EVENT_FLAG_CALIB = 1u << 2      /**< event came from a calibration run */
} evt_flags_t;

/** Classification of an impact, computed on the device. */
typedef enum {
    HIT_CLASS_UNKNOWN = 0,
    HIT_CLASS_ARROW = 1,    /**< confident arrow strike */
    HIT_CLASS_LIGHT = 2,    /**< light touch / graze */
    HIT_CLASS_SHAKE = 3,    /**< the target itself was moved */
    HIT_CLASS_NOISE = 4,    /**< wind gust, rain, distant shot */
    HIT_CLASS_COUNT
} hit_class_t;

/**
 * @brief  CRC-8, polynomial 0x07, initial value 0xFF, no final XOR.
 * @param  data Bytes to checksum.
 * @param  len  Number of bytes.
 * @return CRC byte.
 */
uint8_t protocol_crc8(const uint8_t *data, size_t len);

/**
 * @brief  Fill a packet with the current event data.
 * @param  pkt       Packet to fill.
 * @param  device_id Unique device id.
 * @param  seq       Event counter.
 * @param  peak_mg   Peak vector magnitude in milli-g.
 * @param  impulse   Integral of |a| over the window in mg*s.
 * @param  klass     Classification result.
 * @param  flags     Event flags (evt_flags_t).
 * @param  batt_mv   Battery voltage in millivolts.
 */
void protocol_build(evt_packet_t *pkt, uint16_t device_id, uint32_t seq,
                    uint16_t peak_mg, uint16_t impulse, hit_class_t klass,
                    uint8_t flags, uint16_t batt_mv);

/**
 * @brief  Serialise a packet into a byte buffer.
 * @param  pkt  Packet to serialise.
 * @param  buf  Destination buffer, at least PACKET_SIZE_BYTES.
 * @param  len  Length of the destination buffer.
 * @return Number of bytes written, or 0 on error.
 */
size_t protocol_encode(const evt_packet_t *pkt, uint8_t *buf, size_t len);

/**
 * @brief  Parse a serialised packet and verify its CRC.
 * @param  buf  Source buffer.
 * @param  len  Length of the source buffer.
 * @param  pkt  Parsed packet.
 * @return 0 on success, -1 on a length, magic or CRC error.
 */
int protocol_decode(const uint8_t *buf, size_t len, evt_packet_t *pkt);

#endif /* PROTOCOL_H */
