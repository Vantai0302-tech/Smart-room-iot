#include <stdio.h>
#include <string.h>
#include "dht20.h"
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static void seal(uint8_t *b) { b[6] = dht20_crc8(b, 6); }
int main(void)
{
    uint8_t b[7]; float t, h;
    puts("D1: 25.0C / 50.0%  (hum raw 0x80000, temp raw 0x60000)");
    uint8_t ok[7] = {0x1C, 0x80, 0x00, 0x06, 0x00, 0x00, 0}; seal(ok);
    CHECK(dht20_decode(ok, &t, &h) && t > 24.99f && t < 25.01f && h > 49.99f && h < 50.01f);
    puts("D2: dang ban (bit7=1) -> loai");
    memcpy(b, ok, 7); b[0] |= 0x80; seal(b); CHECK(!dht20_decode(b, &t, &h));
    puts("D3: sai CRC -> loai");
    memcpy(b, ok, 7); b[6] ^= 0x55; CHECK(!dht20_decode(b, &t, &h));
    puts("D4: gia tri vo ly (hum raw toan 1 -> 100% van hop le, temp raw toan 1 = 150C bi loai)");
    uint8_t hot[7] = {0x1C, 0x80, 0x00, 0x0F, 0xFF, 0xFF, 0}; seal(hot); CHECK(!dht20_decode(hot, &t, &h));
    puts("D5: CRC8 vector: 1 byte 0x00 voi init 0xFF");
    uint8_t z = 0; CHECK(dht20_crc8(&z, 1) == 0xAC);
    puts(fails ? "KET QUA: CO LOI" : "KET QUA: TAT CA PASS");
    return fails != 0;
}
