#include "sensor_mock.h"

static float m_t = 25.0f, m_h = 60.0f, m_lux = 300.0f;
static bool m_door, m_btn, m_th_fail, m_lux_fail;

void sensor_mock_set(float t, float h, float lux, bool door) { m_t = t; m_h = h; m_lux = lux; m_door = door; }
void sensor_mock_fail(bool th, bool lux) { m_th_fail = th; m_lux_fail = lux; }
void sensor_mock_button(bool p) { m_btn = p; }
void sensor_mock_door_raw(bool o) { m_door = o; }

static bool rd_th(float *t, float *h) { if (m_th_fail) return false; *t = m_t; *h = m_h; return true; }
static bool rd_lux(float *l) { if (m_lux_fail) return false; *l = m_lux; return true; }
static bool rd_door(bool *o) { *o = m_door; return true; }
static bool rd_btn(bool *p) { *p = m_btn; return true; }

sensor_sources_t sensor_mock_sources(void)
{
    sensor_sources_t s = { rd_th, rd_lux, rd_door, rd_btn };
    return s;
}
