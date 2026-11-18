/**
 * @file    test_protocol.c
 * @brief   Host tests for the event packet format and CRC-8.
 *
 * Build (no hardware, no vendor SDK):
 *   gcc -I firmware/include firmware/src/protocol.c firmware/tests/test_protocol.c \
 *       -o firmware/build/test_protocol && ./firmware/build/test_protocol
 */

#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "config.h"

static int g_failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
            g_failures++;                                                  \
        }                                                                  \
    } while (0)

static void test_crc8(void)
{
    /* CRC-8/SMBUS check value: CRC over "123456789" is 0xF4. */
    const uint8_t check[] = "123456789";
    CHECK(protocol_crc8(check, 9u) == 0xF4u);

    /* CRC of an empty buffer is the init value. */
    CHECK(protocol_crc8(check, 0u) == 0xFFu);

    /* Changing one bit changes the CRC. */
    {
        uint8_t a[4] = { 0x00u, 0x11u, 0x22u, 0x33u };
        uint8_t b[4] = { 0x00u, 0x11u, 0x22u, 0x34u };
        CHECK(protocol_crc8(a, 4u) != protocol_crc8(b, 4u));
    }
}

static void test_roundtrip(void)
{
    evt_packet_t pkt;
    evt_packet_t out;
    uint8_t buf[PACKET_SIZE_BYTES];

    protocol_build(&pkt, 0x1234u, 42u, 12345u, 777u, HIT_CLASS_ARROW,
                   EVENT_FLAG_SATURATED, 3987u);
    CHECK(protocol_encode(&pkt, buf, sizeof(buf)) == PACKET_SIZE_BYTES);
    CHECK(buf[0] == PACKET_MAGIC);
    CHECK(protocol_decode(buf, sizeof(buf), &out) == 0);
    CHECK(out.device_id == 0x1234u);
    CHECK(out.seq == 42u);
    CHECK(out.peak_mg == 12345u);
    CHECK(out.impulse == 777u);
    CHECK(out.hit_class == (uint8_t)HIT_CLASS_ARROW);
    CHECK((out.flags & EVENT_FLAG_SATURATED) != 0u);
    CHECK(out.batt_mv == 3987u);
    CHECK(out.version == PACKET_VERSION);
}

static void test_corruption_is_detected(void)
{
    evt_packet_t pkt;
    evt_packet_t out;
    uint8_t buf[PACKET_SIZE_BYTES];

    protocol_build(&pkt, 0x00ABu, 7u, 100u, 20u, HIT_CLASS_UNKNOWN, 0u, 3700u);
    (void)protocol_encode(&pkt, buf, sizeof(buf));

    /* Flip a bit in the payload. */
    buf[8] = (uint8_t)(buf[8] ^ 0x01u);
    CHECK(protocol_decode(buf, sizeof(buf), &out) == -1);

    /* Wrong magic. */
    (void)protocol_encode(&pkt, buf, sizeof(buf));
    buf[0] = 0x00u;
    CHECK(protocol_decode(buf, sizeof(buf), &out) == -1);

    /* Too short. */
    (void)protocol_encode(&pkt, buf, sizeof(buf));
    CHECK(protocol_decode(buf, sizeof(buf) - 1u, &out) == -1);
}

static void test_short_buffer_is_rejected(void)
{
    evt_packet_t pkt;
    uint8_t buf[PACKET_SIZE_BYTES];

    protocol_build(&pkt, 1u, 1u, 1u, 1u, HIT_CLASS_UNKNOWN, 0u, 1u);
    CHECK(protocol_encode(&pkt, buf, sizeof(buf) - 1u) == 0u);
}

int main(void)
{
    test_crc8();
    test_roundtrip();
    test_corruption_is_detected();
    test_short_buffer_is_rejected();

    if (g_failures == 0) {
        printf("protocol: all tests passed\n");
        return 0;
    }
    printf("protocol: %d test(s) FAILED\n", g_failures);
    return 1;
}
