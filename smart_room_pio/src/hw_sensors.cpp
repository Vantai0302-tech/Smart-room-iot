#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "hw_sensors.h"
#include "sht20_modbus.h"
#include "dht20.h"

#define BH1750_ADDR 0x23
#define BH1750_CONT_HRES 0x10

/* ---- BH1750 ---- */
static bool rd_lux(float *lux)
{
    if (Wire.requestFrom((uint8_t)BH1750_ADDR, (uint8_t)2) != 2) return false;
    uint16_t raw = (uint16_t)(Wire.read() << 8);
    raw |= Wire.read();
    *lux = raw / 1.2f;                       /* công thức datasheet */
    return true;
}

/* ---- Cửa, nút ---- */
static bool rd_door(bool *open)    { *open = digitalRead(PIN_DOOR) != 0; return true; }
static bool rd_button(bool *press) { *press = digitalRead(PIN_BUTTON) == 0; return true; }

/* ---- SHT20 qua Modbus RS485 (Serial1, điều khiển DE bằng tay) ---- */
static void hex(const char *dir, const uint8_t *b, int n)
{
    Serial.printf("[sht20] %s:", dir);
    for (int i = 0; i < n; i++) Serial.printf(" %02X", b[i]);
    Serial.println();
}

static bool rd_temp_hum(float *t, float *h)
{
    uint8_t tx[8], rx[16];
    size_t n = modbus_build_read(SHT20_SLAVE_ADDR, SHT20_FUNC, SHT20_REG_START, SHT20_REG_COUNT, tx);
    while (Serial1.available()) Serial1.read();            /* xả dữ liệu cũ */
    hex("TX", tx, (int)n);
    digitalWrite(PIN_RS485_DE, HIGH);                      /* chuyển sang chiều gửi */
    Serial1.write(tx, n);
    Serial1.flush();                                       /* chờ gửi xong */
    digitalWrite(PIN_RS485_DE, LOW);                       /* về chiều nhận */
    size_t len = Serial1.readBytes(rx, 5 + 2 * SHT20_REG_COUNT);   /* timeout 200 ms */
    if (len == 0) { Serial.println("[sht20] timeout"); return false; }
    hex("RX", rx, (int)len);
    uint16_t regs[SHT20_REG_COUNT];
    mb_status_t st = modbus_parse_read(rx, len, SHT20_SLAVE_ADDR, SHT20_FUNC, SHT20_REG_COUNT, regs);
    if (st != MB_OK) { Serial.printf("[sht20] modbus error %d\n", (int)st); return false; }
    return sht20_decode(regs, t, h);
}

/* ---- DHT20 qua I2C: đọc kết quả đo lần trước, rồi kích đo lần kế (không phải chờ 80 ms) ---- */
static void dht20_trigger(void)
{
    Wire.beginTransmission(DHT20_ADDR);
    Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
    Wire.endTransmission();
}

static bool rd_dht20(float *t, float *h)
{
    uint8_t b[7];
    bool ok = false;
    if (Wire.requestFrom((uint8_t)DHT20_ADDR, (uint8_t)7) == 7) {
        for (int i = 0; i < 7; i++) b[i] = Wire.read();
        ok = dht20_decode(b, t, h);
    } else {
        Serial.println("[dht20] khong thay cam bien (kiem tra day I2C)");
    }
    dht20_trigger();                      /* bắt đầu lần đo tiếp theo */
    return ok;
}

/* ---- Số giả lập (USE_MOCK_SENSORS) ---- */
static bool mock_th(float *t, float *h)
{
    uint32_t s = millis() / 1000;
    if (s % 60 >= 50) return false;                 /* mô phỏng mất cảm biến 10 giây mỗi phút */
    *t = 25.0f + 0.5f * (s % 24);                   /* 25.0 -> 36.5 °C, vượt 33 °C ở giây 16 */
    *h = 60.0f - 0.3f * (s % 24);
    return true;
}
static bool mock_lux(float *l)
{
    *l = 200.0f + 10.0f * ((millis() / 1000) % 30);
    return true;
}

/* ---- Task ---- */
static void sensor_task(void *arg)
{
    for (;;) {
        sensor_manager_poll(millis());
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sensor_hw_start(sensor_sample_cb_t on_sample, sensor_button_cb_t on_button)
{
    pinMode(PIN_DOOR, INPUT_PULLUP);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    sensor_sources_t s = {};
    s.read_door_raw = rd_door;
    s.read_button_raw = rd_button;

#if USE_MOCK_SENSORS
    Serial.println("[hw] CHE DO GIA LAP: nhiet do/am/anh sang la so gia");
    s.read_temp_hum = mock_th;
    s.read_lux = mock_lux;
#else
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    delay(100);                                  /* DHT20 cần >=100 ms sau khi cấp nguồn */
    dht20_trigger();                             /* kích lần đo đầu để 1 giây sau có số liệu */
    Wire.beginTransmission(BH1750_ADDR);
    Wire.write(BH1750_CONT_HRES);
    if (Wire.endTransmission() != 0) Serial.println("[bh1750] khong thay cam bien (kiem tra day I2C)");

    pinMode(PIN_RS485_DE, OUTPUT);
    digitalWrite(PIN_RS485_DE, LOW);
    Serial1.begin(RS485_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    Serial1.setTimeout(200);
  #if USE_DHT20
    s.read_temp_hum = rd_dht20;
  #else
    s.read_temp_hum = rd_temp_hum;
  #endif
    s.read_lux = rd_lux;
#endif

    sensor_manager_init(&s, on_sample, on_button);
    xTaskCreate(sensor_task, "SensorTask", 4096, NULL, 5, NULL);
}
