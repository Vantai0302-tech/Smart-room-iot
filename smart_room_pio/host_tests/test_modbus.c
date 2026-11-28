#include <stdio.h>
#include <string.h>
#include "sht20_modbus.h"
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static size_t make_resp(uint8_t *b, uint16_t t, uint16_t h)
{
    b[0]=1; b[1]=4; b[2]=4; b[3]=t>>8; b[4]=t&255; b[5]=h>>8; b[6]=h&255;
    uint16_t c = modbus_crc16(b, 7); b[7]=c&255; b[8]=c>>8; return 9;
}
int main(void)
{
    uint8_t b[16]; uint16_t r[2]; float t, h;
    /* Vector chuẩn: 01 03 00 00 00 0A -> CRC C5 CD */
    const uint8_t v[] = {0x01,0x03,0x00,0x00,0x00,0x0A};
    CHECK(modbus_crc16(v, 6) == 0xCDC5);

    puts("M1: frame request");
    size_t n = modbus_build_read(1, 4, 1, 2, b);
    CHECK(n == 8 && b[0]==1 && b[1]==4 && b[3]==1 && b[5]==2);
    CHECK((b[6] | (b[7]<<8)) == modbus_crc16(b, 6));

    puts("M2: giai ma 26.5C / 61.2%");
    n = make_resp(b, 265, 612);
    CHECK(modbus_parse_read(b, n, 1, 4, 2, r) == MB_OK);
    CHECK(sht20_decode(r, &t, &h) && t == 26.5f && h > 61.19f && h < 61.21f);

    puts("M3: nhiet do am (-5.0C)");
    n = make_resp(b, (uint16_t)(int16_t)-50, 400);
    CHECK(modbus_parse_read(b, n, 1, 4, 2, r) == MB_OK && sht20_decode(r, &t, &h) && t == -5.0f);

    puts("M4: loi giao tiep");
    n = make_resp(b, 265, 612); b[4] ^= 0xFF;
    CHECK(modbus_parse_read(b, n, 1, 4, 2, r) == MB_ERR_CRC);
    n = make_resp(b, 265, 612);
    CHECK(modbus_parse_read(b, n, 2, 4, 2, r) == MB_ERR_ADDR);
    CHECK(modbus_parse_read(b, 3, 1, 4, 2, r) == MB_ERR_SHORT);
    uint8_t ex[] = {1, 0x84, 2, 0, 0}; uint16_t c = modbus_crc16(ex, 3); ex[3]=c&255; ex[4]=c>>8;
    CHECK(modbus_parse_read(ex, 5, 1, 4, 2, r) == MB_ERR_EXCEPTION);

    puts("M5: gia tri vo ly bi loai");
    r[0] = 2000; r[1] = 500; CHECK(!sht20_decode(r, &t, &h));   /* 200C */
    r[0] = 250;  r[1] = 1500; CHECK(!sht20_decode(r, &t, &h));  /* 150% */

    puts(fails ? "KET QUA: CO LOI" : "KET QUA: TAT CA PASS");
    return fails != 0;
}
