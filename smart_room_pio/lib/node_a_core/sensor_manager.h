#pragma once
#include "sensor_data.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_POLL_PERIOD_MS   1000u /* chu kỳ đọc nhiệt độ/ẩm/ánh sáng */
#define INPUT_DEBOUNCE_MS       30u   /* chống dội cửa và nút */

/* Nguồn dữ liệu: có thể là driver thật hoặc mock. Trả true nếu đọc thành công. */
typedef struct {
    bool (*read_temp_hum)(float *temp_c, float *hum_pct); /* SHT20 (A02) */
    bool (*read_lux)(float *lux);                         /* BH1750 */
    bool (*read_door_raw)(bool *open);
    bool (*read_button_raw)(bool *pressed);
} sensor_sources_t;

typedef void (*sensor_sample_cb_t)(const sensor_data_t *s); /* gửi sang Node B */
typedef void (*sensor_button_cb_t)(void);                   /* sự kiện nhấn nút */

void sensor_manager_init(const sensor_sources_t *src,
                         sensor_sample_cb_t on_sample,
                         sensor_button_cb_t on_button);

/* Gọi đều đặn (khoảng 10 ms/lần) với thời gian hiện tại. Tự quyết khi nào đọc/gửi. */
void sensor_manager_poll(uint32_t now_ms);

bool sensor_is_stale(const sensor_data_t *s, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
