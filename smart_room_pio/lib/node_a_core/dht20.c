#include "dht20.h"

uint8_t dht20_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;                       /* đa thức 0x31, khởi tạo 0xFF theo datasheet */
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

bool dht20_decode(const uint8_t b[7], float *temp_c, float *hum_pct)
{
    if (b[0] & 0x80) return false;                        /* bit 7 = đang đo, chưa có kết quả */
    if (dht20_crc8(b, 6) != b[6]) return false;
    uint32_t hraw = ((uint32_t)b[1] << 12) | ((uint32_t)b[2] << 4) | (b[3] >> 4);
    uint32_t traw = ((uint32_t)(b[3] & 0x0F) << 16) | ((uint32_t)b[4] << 8) | b[5];
    float h = hraw * 100.0f / 1048576.0f;
    float t = traw * 200.0f / 1048576.0f - 50.0f;
    if (t < -40.0f || t > 85.0f || h < 0.0f || h > 100.0f) return false;
    *temp_c = t; *hum_pct = h;
    return true;
}
