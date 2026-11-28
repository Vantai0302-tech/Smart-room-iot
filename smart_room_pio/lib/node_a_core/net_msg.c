#include <stdio.h>
#include <string.h>
#include "net_msg.h"
#include "cJSON.h"

static int print_to(cJSON *root, char *buf, size_t n)
{
    int ok = cJSON_PrintPreallocated(root, buf, (int)n, 0);
    cJSON_Delete(root);
    return ok ? (int)strlen(buf) : -1;
}

int net_encode_telemetry(const sensor_data_t *s, const char *device_id, uint32_t seq, char *buf, size_t n)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) return -1;
    cJSON_AddStringToObject(o, "device_id", device_id);
    cJSON_AddNumberToObject(o, "seq", seq);
    cJSON_AddNumberToObject(o, "temperature_c", s->temperature_c);
    cJSON_AddNumberToObject(o, "humidity_pct", s->humidity_pct);
    cJSON_AddNumberToObject(o, "light_lux", s->light_lux);
    cJSON_AddBoolToObject(o, "door_open", s->door_open);
    cJSON_AddNumberToObject(o, "sample_time_ms", s->sample_time_ms);
    cJSON_AddBoolToObject(o, "sensor_valid", s->sensor_valid);
    return print_to(o, buf, n);
}

int net_encode_result(const net_result_t *r, char *buf, size_t n)
{
    cJSON *o = cJSON_CreateObject();
    if (!o) return -1;
    cJSON_AddStringToObject(o, "command_id", r->command_id);   /* cJSON tự escape ký tự đặc biệt */
    cJSON_AddStringToObject(o, "status", r->status);
    cJSON_AddStringToObject(o, "reason_code", r->reason_code);
    cJSON *a = cJSON_AddObjectToObject(o, "actual_state");
    if (a) { cJSON_AddBoolToObject(a, "fan", r->fan_on); cJSON_AddBoolToObject(a, "light", r->light_on); }
    cJSON_AddNumberToObject(o, "state_version", r->state_version);
    cJSON_AddStringToObject(o, "mode", r->mode);
    cJSON_AddNumberToObject(o, "override_remaining_ms", r->override_remaining_ms);
    return print_to(o, buf, n);
}

static bool safe_id(const char *s)
{
    if (!*s) return false;
    for (; *s; s++) {
        char c = *s;
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              c == '-' || c == '_' || c == '.' || c == ':')) return false;
    }
    return true;
}

/* Chép chuỗi bắt buộc. Trả false nếu thiếu/không phải chuỗi/quá dài. */
static bool get_str(const cJSON *o, const char *key, char *dst, size_t cap)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(o, key);
    if (!cJSON_IsString(it) || !it->valuestring || strlen(it->valuestring) >= cap) return false;
    strcpy(dst, it->valuestring);
    return true;
}

static double get_num(const cJSON *o, const char *key, double def)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(o, key);
    return cJSON_IsNumber(it) ? it->valuedouble : def;
}

net_parse_t net_parse_command(const char *json, size_t len, net_command_t *out)
{
    memset(out, 0, sizeof *out);
    cJSON *o = cJSON_ParseWithLength(json, len);
    if (!o || !cJSON_IsObject(o)) { cJSON_Delete(o); return NET_PARSE_BAD_JSON; }

    net_parse_t res = NET_PARSE_OK;
    if (!get_str(o, "command_id", out->command_id, NET_ID_LEN) || !safe_id(out->command_id)) {
        out->command_id[0] = '\0';
        res = NET_PARSE_MISSING;
    } else if (!get_str(o, "target", out->target, NET_STR_LEN) ||
               !get_str(o, "action", out->action, NET_STR_LEN)) {
        res = NET_PARSE_MISSING;
    } else {
        if (!get_str(o, "source", out->source, NET_STR_LEN)) strcpy(out->source, "gateway");
        double d = get_num(o, "duration_ms", 0);
        if (d < 0 || d > 86400000.0) res = NET_PARSE_BAD_VALUE;
        else out->duration_ms = (uint32_t)d;
        out->issued_at_ms  = (int64_t)get_num(o, "issued_at_ms", 0);
        out->expires_at_ms = (int64_t)get_num(o, "expires_at_ms", 0);
        out->policy_version = (uint32_t)get_num(o, "policy_version", 0);
    }
    cJSON_Delete(o);
    return res;
}

bool net_topic(char *buf, size_t n, const char *device_id, const char *leaf)
{
    int w = snprintf(buf, n, "smartroom/%s/%s", device_id, leaf);
    return w > 0 && (size_t)w < n;
}
