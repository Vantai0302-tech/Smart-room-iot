#pragma once
/* Định dạng bản tin MQTT (A05). Thuần C + cJSON, test được trên PC. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sensor_data.h"
#ifdef __cplusplus
extern "C" {
#endif

#define NET_ID_LEN  40
#define NET_STR_LEN 24

/* Lệnh nhận từ mạng, đúng mục 8.2 của hợp đồng. TV1 tự chuyển sang command_t của Node B. */
typedef struct {
    char     command_id[NET_ID_LEN];
    char     target[NET_STR_LEN];    /* "fan" | "light" */
    char     action[NET_STR_LEN];    /* "ON" | "OFF" */
    char     source[NET_STR_LEN];    /* "cloud" | "web" | "gateway" */
    uint32_t duration_ms;            /* 0 = không có thời hạn do lệnh yêu cầu */
    int64_t  issued_at_ms;           /* unix ms, 0 = không có */
    int64_t  expires_at_ms;          /* unix ms, 0 = không có */
    uint32_t policy_version;
} net_command_t;

/* Kết quả/trạng thái từ Node B, đúng mục 8.3. command_id rỗng = bản tin trạng thái chủ động. */
typedef struct {
    char     command_id[NET_ID_LEN];
    char     status[NET_STR_LEN];        /* APPLIED | REJECTED | ... do Node B quy định */
    char     reason_code[NET_STR_LEN];
    bool     fan_on, light_on;           /* actual_state */
    uint32_t state_version;
    char     mode[NET_STR_LEN];
    uint32_t override_remaining_ms;
} net_result_t;

typedef enum { NET_PARSE_OK = 0, NET_PARSE_BAD_JSON, NET_PARSE_MISSING, NET_PARSE_BAD_VALUE } net_parse_t;

/* Trả về độ dài chuỗi, hoặc -1 nếu buffer nhỏ. */
/* Telemetry theo định dạng gateway (TV3): thêm device_id và seq (seq luôn tăng, kể cả sau khi board reset). */
int net_encode_telemetry(const sensor_data_t *s, const char *device_id, uint32_t seq, char *buf, size_t n);
int net_encode_result(const net_result_t *r, char *buf, size_t n);

/* Phân tích lệnh. Khi lỗi, out->command_id vẫn được điền nếu đọc được (để gửi NACK). */
net_parse_t net_parse_command(const char *json, size_t len, net_command_t *out);

/* Tạo topic: smartroom/<device_id>/<leaf>. Trả về false nếu buffer nhỏ. */
bool net_topic(char *buf, size_t n, const char *device_id, const char *leaf);

#ifdef __cplusplus
}
#endif
