#pragma once
#include "net_msg.h"
/* Chạy trong task mạng: hãy bỏ lệnh vào hàng đợi của Node B, đừng xử lý nặng ở đây. */
typedef void (*net_command_cb_t)(const net_command_t *cmd);

void network_transport_start(net_command_cb_t on_command);
void network_publish_telemetry(const sensor_data_t *s);   /* smartroom/<id>/telemetry */
void network_publish_result(const net_result_t *r);       /* smartroom/<id>/ack */
void network_publish_state(const net_result_t *r);        /* smartroom/<id>/state, retain */
