#include "modbus_rtu.h"

uint16_t modbus_crc16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
    }
    return crc;
}

size_t modbus_build_read(uint8_t addr, uint8_t func, uint16_t reg, uint16_t count, uint8_t *out)
{
    out[0] = addr; out[1] = func;
    out[2] = reg >> 8;   out[3] = reg & 0xFF;
    out[4] = count >> 8; out[5] = count & 0xFF;
    uint16_t crc = modbus_crc16(out, 6);
    out[6] = crc & 0xFF;  /* CRC gửi byte thấp trước */
    out[7] = crc >> 8;
    return 8;
}

mb_status_t modbus_parse_read(const uint8_t *rx, size_t len, uint8_t addr, uint8_t func,
                              uint16_t count, uint16_t *regs_out)
{
    if (len < 5) return MB_ERR_SHORT;
    uint16_t crc = modbus_crc16(rx, len - 2);
    if ((rx[len - 2] | (rx[len - 1] << 8)) != crc) return MB_ERR_CRC;
    if (rx[0] != addr) return MB_ERR_ADDR;
    if (rx[1] == (func | 0x80)) return MB_ERR_EXCEPTION;
    if (rx[1] != func || rx[2] != count * 2 || len != (size_t)(5 + count * 2)) return MB_ERR_FORMAT;
    for (uint16_t i = 0; i < count; i++)
        regs_out[i] = (uint16_t)((rx[3 + 2 * i] << 8) | rx[4 + 2 * i]);
    return MB_OK;
}
