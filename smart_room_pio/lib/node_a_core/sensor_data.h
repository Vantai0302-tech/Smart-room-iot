#pragma once
/* Hợp đồng Node A -> Node B (mục 8.1). Đơn vị cố định, không đổi giữa các module. */
#include <stdbool.h>
#include <stdint.h>

#define SENSOR_STALE_MS 5000u /* quá 5 giây không có mẫu mới => dữ liệu cũ */

typedef struct {
    float    temperature_c;  /* độ C */
    float    humidity_pct;   /* % RH, 0..100 */
    float    light_lux;      /* lux */
    bool     door_open;      /* đã debounce */
    uint32_t sample_time_ms; /* thời điểm lấy mẫu nhiệt độ/ẩm thành công gần nhất (ms từ lúc boot) */
    bool     sensor_valid;   /* false khi nhiệt độ/ẩm hoặc ánh sáng chưa có mẫu hoặc đã quá hạn */
} sensor_data_t;
