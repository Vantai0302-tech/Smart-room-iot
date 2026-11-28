/* Wi-Fi + MQTT bằng Arduino (PubSubClient). Mọi thao tác MQTT nằm trong MỘT task (NetTask);
   nơi khác chỉ bỏ bản tin vào hàng đợi, vì PubSubClient không an toàn khi gọi từ nhiều task.
   Giới hạn: PubSubClient chỉ publish QoS0 (ack/state không còn QoS1) và không cho biết cờ retain
   của lệnh nhận về; vì vậy gateway tuyệt đối không gửi cmd kiểu retain (bridge.py đã tuân thủ). */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "secrets.h"
#include "network_transport.h"

struct Pub { char topic[64]; char payload[384]; bool retain; };

static QueueHandle_t pubq;
static WiFiClient wifi;
static PubSubClient mqtt(wifi);
static net_command_cb_t cmd_cb;
static uint32_t seq_base, seq_n;   /* seq = (số lần boot)*1.000.000 + bộ đếm: không bao giờ lùi sau khi reset */
static char t_telemetry[64], t_state[64], t_ack[64], t_cmd[64], t_status[64];

static void enqueue(const char *topic, const char *payload, bool retain)
{
    if (!pubq) return;
    Pub p;
    strlcpy(p.topic, topic, sizeof p.topic);
    strlcpy(p.payload, payload, sizeof p.payload);
    p.retain = retain;
    xQueueSend(pubq, &p, 0);          /* đầy thì bỏ: ưu tiên không treo SensorTask */
}

void network_publish_telemetry(const sensor_data_t *s)
{
    char buf[256];
    if (net_encode_telemetry(s, DEVICE_ID, seq_base + (++seq_n), buf, sizeof buf) > 0) enqueue(t_telemetry, buf, false);
}
void network_publish_result(const net_result_t *r)
{
    char buf[384];
    if (net_encode_result(r, buf, sizeof buf) > 0) enqueue(t_ack, buf, false);
}
void network_publish_state(const net_result_t *r)
{
    char buf[384];
    if (net_encode_result(r, buf, sizeof buf) > 0) enqueue(t_state, buf, true);
}

static void on_message(char *topic, uint8_t *payload, unsigned int len)
{
    net_command_t c;
    net_parse_t p = net_parse_command((const char *)payload, len, &c);
    if (p == NET_PARSE_OK) { if (cmd_cb) cmd_cb(&c); return; }
    net_result_t r = {};                 /* sai định dạng: NACK ngay tại transport */
    strlcpy(r.command_id, c.command_id, sizeof r.command_id);
    strcpy(r.status, "REJECTED");
    strcpy(r.reason_code, p == NET_PARSE_BAD_VALUE ? "BAD_VALUE" : "BAD_FORMAT");
    Serial.printf("[net] bad command (%d)\n", (int)p);
    network_publish_result(&r);
}

static void net_task(void *arg)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(512);
    mqtt.setSocketTimeout(3);
    mqtt.setCallback(on_message);
    uint32_t last_try = 0;
    bool had_ip = false;

    for (;;) {
        bool wifi_ok = WiFi.status() == WL_CONNECTED;
        if (wifi_ok && !had_ip) Serial.printf("[net] wifi ok, ip=%s\n", WiFi.localIP().toString().c_str());
        had_ip = wifi_ok;

        if (wifi_ok) {
            if (!mqtt.connected()) {
                if (last_try == 0 || millis() - last_try > 3000) {
                    last_try = millis();
                    /* phiên sạch (true): lệnh phát lúc thiết bị offline không bị giao lại */
                    if (mqtt.connect(DEVICE_ID, NULL, NULL, t_status, 1, true, "offline", true)) {
                        mqtt.subscribe(t_cmd, 1);
                        mqtt.publish(t_status, "online", true);
                        Serial.println("[net] mqtt connected");
                    } else {
                        Serial.printf("[net] mqtt fail rc=%d\n", mqtt.state());
                    }
                }
            } else {
                mqtt.loop();
            }
        }
        Pub p;
        while (xQueueReceive(pubq, &p, 0) == pdTRUE)
            if (mqtt.connected()) mqtt.publish(p.topic, p.payload, p.retain);   /* offline thì bỏ */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void network_transport_start(net_command_cb_t on_command)
{
    cmd_cb = on_command;
    Preferences prefs;
    prefs.begin("nodea", false);
    uint32_t boot = prefs.getUInt("boot", 0) + 1;
    prefs.putUInt("boot", boot);
    prefs.end();
    seq_base = boot * 1000000UL;
    net_topic(t_telemetry, sizeof t_telemetry, DEVICE_ID, "telemetry");
    net_topic(t_state,     sizeof t_state,     DEVICE_ID, "state");
    net_topic(t_ack,       sizeof t_ack,       DEVICE_ID, "ack");
    net_topic(t_cmd,       sizeof t_cmd,       DEVICE_ID, "cmd");
    net_topic(t_status,    sizeof t_status,    DEVICE_ID, "status");
    pubq = xQueueCreate(8, sizeof(Pub));
    xTaskCreate(net_task, "NetTask", 6144, NULL, 3, NULL);
}
