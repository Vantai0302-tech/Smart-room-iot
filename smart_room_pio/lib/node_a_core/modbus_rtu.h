#pragma once
/* Modbus RTU thuần C (không phụ thuộc ESP-IDF) để test được trên PC. */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MB_OK = 0,
    MB_ERR_SHORT,      /* thiếu byte */
    MB_ERR_CRC,        /* sai CRC */
    MB_ERR_ADDR,       /* sai địa chỉ slave */
    MB_ERR_EXCEPTION,  /* slave trả mã lỗi Modbus */
    MB_ERR_FORMAT,     /* sai function/độ dài */
} mb_status_t;

uint16_t modbus_crc16(const uint8_t *buf, size_t len);

/* Tạo frame đọc thanh ghi (8 byte). Trả về số byte. */
size_t modbus_build_read(uint8_t addr, uint8_t func, uint16_t reg, uint16_t count, uint8_t *out);

/* Giải mã phản hồi đọc. regs_out cần chứa được `count` phần tử. */
mb_status_t modbus_parse_read(const uint8_t *rx, size_t len, uint8_t addr, uint8_t func,
                              uint16_t count, uint16_t *regs_out);

#ifdef __cplusplus
}
#endif
