#include <string.h>
#include "sensor_manager.h"
#include "input_filter.h"

static sensor_sources_t src;
static sensor_sample_cb_t sample_cb;
static sensor_button_cb_t button_cb;
static sensor_data_t last;
static debounce_t door_db;
static button_t btn;
static bool started, th_seen, lux_seen;
static uint32_t last_poll_ms, last_lux_ms;

bool sensor_is_stale(const sensor_data_t *s, uint32_t now_ms)
{
    return (uint32_t)(now_ms - s->sample_time_ms) > SENSOR_STALE_MS;
}

void sensor_manager_init(const sensor_sources_t *s, sensor_sample_cb_t on_sample, sensor_button_cb_t on_button)
{
    memset(&last, 0, sizeof last);
    memset(&door_db, 0, sizeof door_db);
    memset(&btn, 0, sizeof btn);
    started = th_seen = lux_seen = false;
    src = *s;
    sample_cb = on_sample;
    button_cb = on_button;
}

void sensor_manager_poll(uint32_t now_ms)
{
    bool raw, door_changed = false;

    if (src.read_button_raw && src.read_button_raw(&raw) &&
        button_update(&btn, raw, now_ms, INPUT_DEBOUNCE_MS) && button_cb)
        button_cb();

    if (src.read_door_raw && src.read_door_raw(&raw)) {
        bool s = debounce_update(&door_db, raw, now_ms, INPUT_DEBOUNCE_MS);
        if (!started || s != last.door_open) door_changed = true;
        last.door_open = s;
    }

    bool due = !started || (uint32_t)(now_ms - last_poll_ms) >= SENSOR_POLL_PERIOD_MS;
    if (!due && !door_changed) return;

    if (due) {
        started = true;
        last_poll_ms = now_ms;
        float t, h, lux;
        if (src.read_temp_hum && src.read_temp_hum(&t, &h)) {
            last.temperature_c = t;
            last.humidity_pct  = h;
            last.sample_time_ms = now_ms;  /* chỉ cập nhật khi đọc thành công */
            th_seen = true;
        }
        if (src.read_lux && src.read_lux(&lux)) {
            last.light_lux = lux;
            last_lux_ms = now_ms;
            lux_seen = true;
        }
    }

    bool lux_stale = (uint32_t)(now_ms - last_lux_ms) > SENSOR_STALE_MS;
    last.sensor_valid = th_seen && lux_seen && !sensor_is_stale(&last, now_ms) && !lux_stale;
    if (sample_cb) sample_cb(&last);
}
