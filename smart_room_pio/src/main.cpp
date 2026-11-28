/* Chương trình thử Node A. "Node B" là task GIẢ (StubControl) chỉ để thử đường truyền;
   khi TV1 xong Node B thật thì thay bằng control_submit_command()/control_update_sensor(). */
#include <Arduino.h>
#include <string.h>
#include "hw_sensors.h"
#include "network_transport.h"

typedef enum { EV_COMMAND, EV_BUTTON, EV_HOT } ev_type_t;
typedef struct { ev_type_t type; net_command_t cmd; } ev_t;

static QueueHandle_t q;
static bool fan_on, light_on;
static uint32_t state_version;
static char mode[NET_STR_LEN] = "MANUAL";

/* ---- Callback từ Node A (chạy ở task khác, chỉ bỏ sự kiện vào hàng đợi) ---- */
static void on_command(const net_command_t *c)
{
    ev_t e = {};
    e.type = EV_COMMAND;
    e.cmd = *c;
    if (xQueueSend(q, &e, 0) != pdTRUE) {
        net_result_t r = {};
        strlcpy(r.command_id, c->command_id, sizeof r.command_id);
        strcpy(r.status, "REJECTED");
        strcpy(r.reason_code, "QUEUE_FULL");
        network_publish_result(&r);
    }
}

static void on_button(void)
{
    ev_t e = {};
    e.type = EV_BUTTON;
    xQueueSend(q, &e, 0);
}

static void on_sample(const sensor_data_t *s)
{
    static bool hot;
    network_publish_telemetry(s);
    Serial.printf("T=%.1fC H=%.1f%% lux=%.0f door=%s valid=%d\n", s->temperature_c, s->humidity_pct,
                  s->light_lux, s->door_open ? "OPEN" : "closed", (int)s->sensor_valid);
    if (s->sensor_valid && !hot && s->temperature_c >= 33.0f) {        /* cạnh lên 33°C */
        hot = true;
        ev_t e = {};
        e.type = EV_HOT;
        xQueueSend(q, &e, 0);
    } else if (hot && s->temperature_c <= 31.0f) {
        hot = false;                                                   /* xuống 31°C mới tính lại */
    }
}

/* ---- Node B giả ---- */
static void fill_state(net_result_t *r)
{
    r->fan_on = fan_on;
    r->light_on = light_on;
    r->state_version = state_version;
    strlcpy(r->mode, mode, sizeof r->mode);
}

static void control_task(void *arg)
{
    ev_t e;
    for (;;) {
        bool got = xQueueReceive(q, &e, pdMS_TO_TICKS(10000)) == pdTRUE;
        net_result_t r = {};
        if (!got) {                         /* 10 giây yên ắng: công bố lại trạng thái */
            fill_state(&r);
            network_publish_state(&r);
            continue;
        }
        if (e.type == EV_COMMAND) {
            bool on = !strcmp(e.cmd.action, "ON");
            strlcpy(r.command_id, e.cmd.command_id, sizeof r.command_id);
            if (strcmp(e.cmd.action, "ON") && strcmp(e.cmd.action, "OFF")) {
                strcpy(r.status, "REJECTED"); strcpy(r.reason_code, "UNKNOWN_ACTION");
            } else if (!strcmp(e.cmd.target, "fan")) {
                fan_on = on; state_version++; strcpy(mode, "MANUAL");
                strcpy(r.status, "APPLIED"); strcpy(r.reason_code, "OK");
            } else if (!strcmp(e.cmd.target, "light")) {
                light_on = on; state_version++; strcpy(mode, "MANUAL");
                strcpy(r.status, "APPLIED"); strcpy(r.reason_code, "OK");
            } else {
                strcpy(r.status, "REJECTED"); strcpy(r.reason_code, "UNKNOWN_TARGET");
            }
            Serial.printf("cmd %s %s %s -> %s\n", e.cmd.command_id, e.cmd.target, e.cmd.action, r.status);
            fill_state(&r);
            network_publish_result(&r);
            network_publish_state(&r);
        } else {
            if (e.type == EV_BUTTON) { light_on = !light_on; strcpy(mode, "MANUAL"); Serial.printf("button -> light %d\n", (int)light_on); }
            else                     { fan_on = true; strcpy(mode, "AUTO"); Serial.println("AUTO: nong qua -> bat quat"); }
            state_version++;
            fill_state(&r);
            network_publish_state(&r);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);                            /* chờ USB-CDC kịp mở để thấy log đầu */
    q = xQueueCreate(8, sizeof(ev_t));
    xTaskCreate(control_task, "StubControl", 4096, NULL, 4, NULL);
    network_transport_start(on_command);
    sensor_hw_start(on_sample, on_button);
    Serial.println("Node A started");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
