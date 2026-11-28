# MQTT schema v1 (TV2 định nghĩa; TV1, TV3 duyệt)

Broker cục bộ trên PC Gateway. `<id>` = device id (mặc định `nodeA`, khớp gateway TV3).
`seq` luôn tăng kể cả sau khi board reset: seq = (số lần boot lưu trong NVS) × 1.000.000 + bộ đếm.

| Topic | Hướng | QoS | Retain | Nội dung |
|---|---|---|---|---|
| `smartroom/<id>/telemetry` | thiết bị → gateway | 0 | không | SensorData (mục 8.1) + `device_id` + `seq` (theo gateway TV3) |
| `smartroom/<id>/state` | thiết bị → gateway | 1 | có | Trạng thái thực tế + `state_version`, công bố lại khi reconnect |
| `smartroom/<id>/ack` | thiết bị → gateway | 1 | không | Kết quả của từng lệnh (CommandResult) |
| `smartroom/<id>/cmd` | gateway → thiết bị | 1 | **không bao giờ** | Command (mục 8.2) |
| `smartroom/<id>/status` | thiết bị → gateway | 1 | có | `online` / `offline` (Last Will) |

## Quy tắc chống lệnh trễ / replay
- `cmd` không retain; thiết bị dùng phiên sạch, nên lệnh phát lúc offline không bị giao lại. Bản tin retain tới thiết bị bị bỏ.
- Mọi lệnh mạng có `command_id` duy nhất và `expires_at_ms` (unix ms). Thiết bị chuyển `expires_at_ms` cho Node B; Node B quyết định xử lý hết hạn/trùng lặp.
- Lệnh sai định dạng bị NACK ngay tại transport với `reason_code` = `BAD_FORMAT` hoặc `BAD_VALUE` (không tới Node B). **TV1 cần thêm 2 mã này vào bảng reason code.**

## Ví dụ
```
cmd:  {"command_id":"c-0001","target":"fan","action":"OFF","source":"cloud","duration_ms":60000,"expires_at_ms":1790000010000,"policy_version":1}
ack:  {"command_id":"c-0001","status":"APPLIED","reason_code":"OK","actual_state":{"fan":false,"light":false},"state_version":7,"mode":"STUDY","override_remaining_ms":60000}
```
`status`, `reason_code`, `mode` là chuỗi do Node B quy định. "Received" khác "Applied": chỉ gửi `APPLIED` sau khi Node B đã áp quyết định.

## Ghi chú cho bản Arduino/PlatformIO
- Thư viện PubSubClient chỉ publish QoS0, nên `ack` và `state` từ thiết bị chạy QoS0 (bảng trên là mức mục tiêu). Thiết bị vẫn subscribe `cmd` ở QoS1.
- PubSubClient không cho biết cờ retain của lệnh nhận về, nên quy tắc "bỏ lệnh retain" dựa vào việc gateway không bao giờ publish `cmd` kiểu retain, cộng với `expires_at_ms`.
