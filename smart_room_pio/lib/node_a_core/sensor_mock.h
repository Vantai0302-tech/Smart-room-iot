#pragma once
#include "sensor_manager.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Dữ liệu giả để test khi chưa có board. */
void sensor_mock_set(float temp_c, float hum_pct, float lux, bool door_open);
void sensor_mock_fail(bool temp_hum_fail, bool lux_fail); /* giả lập cảm biến hỏng/mất */
void sensor_mock_button(bool pressed_raw);                /* giả lập tín hiệu nút (cả dội) */
void sensor_mock_door_raw(bool open_raw);                 /* giả lập tín hiệu cửa thô */
sensor_sources_t sensor_mock_sources(void);

#ifdef __cplusplus
}
#endif
