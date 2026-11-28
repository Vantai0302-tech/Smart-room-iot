/* Test Node A tren PC (xem run_test.sh) */
#include <stdio.h>
#include "sensor_manager.h"
#include "sensor_mock.h"

/* ---- Engine GIẢ chỉ để test (Node B thật do TV1 viết) ---- */
static int auto_fan_on, samples, presses;
static sensor_data_t got;
static void fake_engine_update(const sensor_data_t *s)
{
    samples++; got = *s;
    if (s->sensor_valid && s->temperature_c >= 33.0f) auto_fan_on++; /* AUTO proposal: FAN ON */
}
static void on_btn(void) { presses++; }

static int fails;
#define CHECK(c) do { if (!(c)) { printf("  FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void run(uint32_t from, uint32_t to) { for (uint32_t t = from; t <= to; t += 10) sensor_manager_poll(t); }
static void setup(void)
{
    sensor_sources_t s = sensor_mock_sources();
    sensor_mock_set(25, 60, 300, false); sensor_mock_fail(false, false);
    sensor_mock_button(false); auto_fan_on = samples = presses = 0;
    sensor_manager_init(&s, fake_engine_update, on_btn);
}

int main(void)
{
    puts("T1: 33C hop le -> AUTO proposal FAN ON");
    setup(); sensor_mock_set(33, 55, 200, false); run(0, 1100);
    CHECK(auto_fan_on >= 1); CHECK(got.sensor_valid); CHECK(got.temperature_c == 33.0f);

    puts("T2: 25C -> khong de xuat");
    setup(); run(0, 1100);
    CHECK(auto_fan_on == 0); CHECK(samples == 2);   /* t=0 va t=1000 */

    puts("T3: nut nhan doi (dội) chi tinh 1 lan nhan");
    setup(); run(0, 100);
    sensor_mock_button(true);  run(110, 120);
    sensor_mock_button(false); run(130, 140);        /* dội */
    sensor_mock_button(true);  run(150, 300);        /* giữ ổn định */
    CHECK(presses == 1);
    sensor_mock_button(false); run(310, 500);
    sensor_mock_button(true);  run(510, 700);        /* nhấn lần 2 */
    CHECK(presses == 2);

    puts("T4: cua doi trang thai -> gui ngay, khong cho het chu ky");
    setup(); run(0, 100); int before = samples;
    sensor_mock_door_raw(true); run(110, 200);
    CHECK(samples == before + 1); CHECK(got.door_open);

    puts("T5: cam bien nhiet do mat -> sau 5s sensor_valid=false, sample_time giu nguyen");
    setup(); run(0, 2000);
    CHECK(got.sensor_valid); uint32_t last_ok = got.sample_time_ms;
    sensor_mock_fail(true, false); run(2010, 8000);
    CHECK(!got.sensor_valid); CHECK(got.sample_time_ms == last_ok);

    puts("T6: chua co mau nao -> invalid");
    setup(); sensor_mock_fail(true, true); run(0, 1100);
    CHECK(!got.sensor_valid);

    puts(fails ? "KET QUA: CO LOI" : "KET QUA: TAT CA PASS");
    return fails != 0;
}
