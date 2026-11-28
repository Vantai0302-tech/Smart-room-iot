#include "sht20_modbus.h"

bool sht20_decode(const uint16_t regs[2], float *temp_c, float *hum_pct)
{
    float t = (int16_t)regs[0] / 10.0f;   /* nhiệt độ có thể âm nên ép kiểu có dấu */
    float h = (int16_t)regs[1] / 10.0f;
    if (t < -40.0f || t > 125.0f || h < 0.0f || h > 100.0f) return false; /* ngoài dải SHT20 */
    *temp_c = t; *hum_pct = h;
    return true;
}
