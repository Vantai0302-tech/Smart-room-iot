#include "input_filter.h"

bool debounce_update(debounce_t *d, bool raw, uint32_t now_ms, uint32_t window_ms)
{
    if (!d->inited) {            /* lần đầu: tin luôn giá trị đọc được */
        d->inited = true;
        d->stable = d->last_raw = raw;
        d->last_change_ms = now_ms;
        return d->stable;
    }
    if (raw != d->last_raw) {    /* tín hiệu vừa đổi: bắt đầu đếm lại */
        d->last_raw = raw;
        d->last_change_ms = now_ms;
    } else if (raw != d->stable && (uint32_t)(now_ms - d->last_change_ms) >= window_ms) {
        d->stable = raw;         /* giữ đủ lâu: chấp nhận */
    }
    return d->stable;
}

bool button_update(button_t *b, bool pressed_raw, uint32_t now_ms, uint32_t window_ms)
{
    bool prev = b->db.inited ? b->db.stable : false;
    bool cur  = debounce_update(&b->db, pressed_raw, now_ms, window_ms);
    return !prev && cur;
}
