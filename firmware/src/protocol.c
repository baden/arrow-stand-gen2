/**
 * @file    protocol.c
 * @brief   Event packet serialisation and CRC-8.
 *
 * No hardware dependencies: this file is compiled into the host tests.
 */

#include "protocol.h"
#include "config.h"

uint8_t protocol_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFu;
    size_t i;

    if (data == NULL) {
        return crc;
    }

    for (i = 0u; i < len; ++i) {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0u; bit < 8u; ++bit) {
            if ((crc & 0x80u) != 0u) {
                crc = (uint8_t)((crc << 1) ^ 0x07u);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

void protocol_build(evt_packet_t *pkt, uint16_t device_id, uint32_t seq,
                    uint16_t peak_mg, uint16_t impulse, hit_class_t klass,
                    uint8_t flags, uint16_t batt_mv)
{
    pkt->magic     = PACKET_MAGIC;
    pkt->version   = PACKET_VERSION;
    pkt->device_id = device_id;
    pkt->seq       = seq;
    pkt->peak_mg   = peak_mg;
    pkt->impulse   = impulse;
    pkt->hit_class = (uint8_t)klass;
    pkt->flags     = flags;
    pkt->batt_mv   = batt_mv;
    pkt->crc       = 0u;
}

size_t protocol_encode(const evt_packet_t *pkt, uint8_t *buf, size_t len)
{
    if (pkt == NULL || buf == NULL || len < PACKET_SIZE_BYTES) {
        return 0u;
    }

    buf[0]  = pkt->magic;
    buf[1]  = pkt->version;
    buf[2]  = (uint8_t)(pkt->device_id & 0xFFu);
    buf[3]  = (uint8_t)((pkt->device_id >> 8) & 0xFFu);
    buf[4]  = (uint8_t)(pkt->seq & 0xFFu);
    buf[5]  = (uint8_t)((pkt->seq >> 8) & 0xFFu);
    buf[6]  = (uint8_t)((pkt->seq >> 16) & 0xFFu);
    buf[7]  = (uint8_t)((pkt->seq >> 24) & 0xFFu);
    buf[8]  = (uint8_t)(pkt->peak_mg & 0xFFu);
    buf[9]  = (uint8_t)((pkt->peak_mg >> 8) & 0xFFu);
    buf[10] = (uint8_t)(pkt->impulse & 0xFFu);
    buf[11] = (uint8_t)((pkt->impulse >> 8) & 0xFFu);
    buf[12] = pkt->hit_class;
    buf[13] = pkt->flags;
    buf[14] = (uint8_t)(pkt->batt_mv & 0xFFu);
    buf[15] = (uint8_t)((pkt->batt_mv >> 8) & 0xFFu);

    buf[PACKET_SIZE_BYTES - 1u] = protocol_crc8(buf, PACKET_SIZE_BYTES - 1u);
    return PACKET_SIZE_BYTES;
}

int protocol_decode(const uint8_t *buf, size_t len, evt_packet_t *pkt)
{
    if (buf == NULL || pkt == NULL || len < PACKET_SIZE_BYTES) {
        return -1;
    }
    if (buf[0] != PACKET_MAGIC) {
        return -1;
    }
    if (protocol_crc8(buf, PACKET_SIZE_BYTES - 1u) !=
        buf[PACKET_SIZE_BYTES - 1u]) {
        return -1;
    }

    pkt->magic     = buf[0];
    pkt->version   = buf[1];
    pkt->device_id = (uint16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    pkt->seq       = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) |
                     ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
    pkt->peak_mg   = (uint16_t)(buf[8] | ((uint16_t)buf[9] << 8));
    pkt->impulse   = (uint16_t)(buf[10] | ((uint16_t)buf[11] << 8));
    pkt->hit_class = buf[12];
    pkt->flags     = buf[13];
    pkt->batt_mv   = (uint16_t)(buf[14] | ((uint16_t)buf[15] << 8));
    pkt->crc       = buf[PACKET_SIZE_BYTES - 1u];
    return 0;
}
