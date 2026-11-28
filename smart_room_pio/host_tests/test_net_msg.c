#include <stdio.h>
#include <string.h>
#include "net_msg.h"
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
#define P(s, out) net_parse_command(s, strlen(s), &out)

int main(void)
{
    char buf[512]; net_command_t c; net_result_t r; sensor_data_t s = {33.0f, 55.5f, 200.0f, true, 1234, true};

    puts("N1: encode telemetry");
    CHECK(net_encode_telemetry(&s, "nodeA", 1000001u, buf, sizeof buf) > 0);
    CHECK(strstr(buf, "\"device_id\":\"nodeA\"") && strstr(buf, "\"seq\":1000001,"));   /* seq là số nguyên, không có dấu chấm/mũ */
    CHECK(strstr(buf, "\"temperature_c\":33") && strstr(buf, "\"door_open\":true") && strstr(buf, "\"sample_time_ms\":1234"));
    CHECK(net_encode_telemetry(&s, "nodeA", 1u, buf, 10) == -1);   /* buffer nhỏ */

    puts("N2: parse lenh hop le");
    CHECK(P("{\"command_id\":\"c-1\",\"target\":\"fan\",\"action\":\"OFF\",\"source\":\"cloud\","
            "\"duration_ms\":60000,\"expires_at_ms\":1760000000000,\"policy_version\":1}", c) == NET_PARSE_OK);
    CHECK(!strcmp(c.command_id, "c-1") && !strcmp(c.target, "fan") && !strcmp(c.action, "OFF"));
    CHECK(c.duration_ms == 60000 && c.expires_at_ms == 1760000000000LL && c.policy_version == 1);

    puts("N3: lenh toi thieu -> mac dinh");
    CHECK(P("{\"command_id\":\"x\",\"target\":\"light\",\"action\":\"ON\"}", c) == NET_PARSE_OK);
    CHECK(!strcmp(c.source, "gateway") && c.duration_ms == 0 && c.expires_at_ms == 0);

    puts("N4: lenh loi");
    CHECK(P("not json", c) == NET_PARSE_BAD_JSON);
    CHECK(P("[1,2]", c) == NET_PARSE_BAD_JSON);
    CHECK(P("{\"target\":\"fan\",\"action\":\"ON\"}", c) == NET_PARSE_MISSING);
    CHECK(P("{\"command_id\":\"a\",\"target\":\"fan\"}", c) == NET_PARSE_MISSING && !strcmp(c.command_id, "a"));
    CHECK(P("{\"command_id\":\"a\",\"target\":\"fan\",\"action\":\"ON\",\"duration_ms\":-5}", c) == NET_PARSE_BAD_VALUE);
    CHECK(P("{\"command_id\":\"a\\\"b\",\"target\":\"fan\",\"action\":\"ON\"}", c) == NET_PARSE_MISSING); /* ký tự lạ trong id */
    CHECK(P("{\"command_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"target\":\"fan\",\"action\":\"ON\"}", c) == NET_PARSE_MISSING);

    puts("N5: encode ket qua (Received != Applied: chi bao theo status tu Node B)");
    memset(&r, 0, sizeof r);
    strcpy(r.command_id, "c-1"); strcpy(r.status, "REJECTED"); strcpy(r.reason_code, "MANUAL_OVERRIDE");
    r.fan_on = false; r.state_version = 7; strcpy(r.mode, "STUDY"); r.override_remaining_ms = 42000;
    CHECK(net_encode_result(&r, buf, sizeof buf) > 0);
    CHECK(strstr(buf, "\"status\":\"REJECTED\"") && strstr(buf, "\"state_version\":7") &&
          strstr(buf, "\"actual_state\":{\"fan\":false,\"light\":false}") && strstr(buf, "\"override_remaining_ms\":42000"));

    puts("N6: topic");
    CHECK(net_topic(buf, sizeof buf, "room1", "cmd") && !strcmp(buf, "smartroom/room1/cmd"));
    CHECK(!net_topic(buf, 8, "room1", "cmd"));

    puts(fails ? "KET QUA: CO LOI" : "KET QUA: TAT CA PASS");
    return fails != 0;
}
