#pragma once
#include <stdbool.h>
#include "modbus_rtu.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Cấu hình mặc định theo module SHT20 RS485 phổ biến (slave 1, input register 0x0001-0x0002,
   giá trị nguyên có dấu, chia 10). PHẢI đối chiếu với datasheet module của nhóm. */
#define SHT20_SLAVE_ADDR   0x01
#define SHT20_FUNC         0x04
#define SHT20_REG_START    0x0001
#define SHT20_REG_COUNT    2

/* Thuần tính toán: từ 2 thanh ghi ra độ C và %RH. Trả false nếu ngoài khoảng hợp lý. */
bool sht20_decode(const uint16_t regs[2], float *temp_c, float *hum_pct);


#ifdef __cplusplus
}
#endif
