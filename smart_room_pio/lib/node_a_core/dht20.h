#pragma once
/* Cảm biến DHT20 (AHT20) qua I2C, địa chỉ 0x38. Phần giải mã thuần C để test trên PC. */
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DHT20_ADDR 0x38

uint8_t dht20_crc8(const uint8_t *data, int len);

/* b[7]: byte trạng thái + 5 byte dữ liệu + CRC. Trả false nếu đang bận, sai CRC hoặc giá trị vô lý. */
bool dht20_decode(const uint8_t b[7], float *temp_c, float *hum_pct);

#ifdef __cplusplus
}
#endif
