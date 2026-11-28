# Gateway PC (T08)

Board (Node A) ──MQTT──> Mosquitto trên PC ──> `src/main.py` ──> CoreIOT, và ngược lại cho lệnh điều khiển.

## Thành phần (`src/`)
- `telemetry_validator.py` – kiểm tra telemetry (giữ nguyên của TV3).
- `coreiot_client.py` – kết nối CoreIOT bằng access token: gửi telemetry, nhận RPC, tự nối lại.
- `bridge_core.py` – dịch bản tin hai chiều, ghép RPC với ACK, từ chối lệnh khi thiết bị offline, báo `sensor_ok`.
- `main.py` – nối tất cả lại.

## Chạy
1. Bật broker cục bộ (xem dự án firmware: `gateway/mosquitto/`).
2. Đặt token (PowerShell, ví dụ):
   ```
   $env:COREIOT_ACCESS_TOKEN="<token của device trên CoreIOT>"
   pip install -r gateway/requirements.txt
   cd gateway/src
   python main.py
   ```
3. Không có board: chạy `python gateway/tests/mock_device.py` để giả lập telemetry.

## Test
`python gateway/tests/test_telemetry_validator.py` và `python gateway/tests/test_bridge_core.py`

## Hợp đồng bản tin
- Thiết bị → gateway: `smartroom/<DEVICE_ID>/{telemetry,state,ack,status}`; gateway → thiết bị: `smartroom/<DEVICE_ID>/cmd` (không retain).
- Telemetry có `device_id`, `seq` (luôn tăng, kể cả sau khi board reset), các trường cảm biến và `sensor_valid`.
- RPC từ CoreIOT: method `setFan` / `setLight`, params `true|false` hoặc `{"value": true, "duration_ms": 60000}`.
- CoreIOT nhận thêm thuộc tính: `fan`, `light`, `mode`, `state_version`, `device_online`, `sensor_ok`.
