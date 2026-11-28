#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Chống dội: tín hiệu phải giữ nguyên đủ window_ms mới được tin. */
typedef struct {
    bool     inited, stable, last_raw;
    uint32_t last_change_ms;
} debounce_t;

bool debounce_update(debounce_t *d, bool raw, uint32_t now_ms, uint32_t window_ms);

/* Nút nhấn: trả true đúng 1 lần mỗi lần nhấn (cạnh lên của trạng thái đã debounce). */
typedef struct { debounce_t db; } button_t;
bool button_update(button_t *b, bool pressed_raw, uint32_t now_ms, uint32_t window_ms);

#ifdef __cplusplus
}
#endif
