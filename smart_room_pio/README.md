# Smart Room – Node A (YoloUNO ESP32-S3, PlatformIO + Arduino)

Phần firmware Node A: đọc cảm biến (DHT20 I2C hoặc SHT20 Modbus RS485, chọn bằng `USE_DHT20` trong `include/pins.h`, BH1750, cửa, nút), gửi MQTT, nhận lệnh, kèm cầu nối Gateway.
Node B (Control Engine) do TV1 viết; `src/main.cpp` hiện dùng Node B GIẢ để thử đường truyền.

## Cấu trúc
- `lib/node_a_core/` – lõi C thuần (Modbus, bộ lọc nút/cửa, SensorManager, JSON). Test được trên PC.
- `src/` – phần dán phần cứng Arduino: `hw_sensors.cpp`, `network_transport.cpp`, `main.cpp`.
- `include/pins.h` – chân GPIO (**đối chiếu sơ đồ board rồi sửa**), `include/secrets.h` – Wi-Fi/MQTT (không commit).
- `gateway/mosquitto/` – cấu hình broker cục bộ. Ứng dụng Gateway (Python) nằm trong repo gateway của TV3. `shared/mqtt_schema.md` – schema topic/JSON.
- `host_tests/` – test trên PC.

## Chạy cả hệ thống (làm theo thứ tự)
**Chuẩn bị chung:** PC và board nối cùng một Wi-Fi (2.4 GHz). Wi-Fi công cộng/trường thường chặn máy nói chuyện với nhau, hãy dùng hotspot điện thoại.

1. **Bật broker MQTT trên PC** (chọn một cách):
   - Docker: `cd gateway/mosquitto && docker compose up`
   - Windows có Mosquitto: bấm đúp `gateway/mosquitto/run_broker_windows.bat` (cần quyền Admin lần đầu để mở tường lửa: `netsh advfirewall firewall add rule name="Mosquitto" dir=in action=allow protocol=TCP localport=1883`)
   - Linux/macOS: `sh gateway/mosquitto/run_broker.sh`
2. **Cấu hình board:** sao chép `include/secrets.example.h` thành `include/secrets.h`, điền Wi-Fi và **IP của PC** (`ipconfig` / `ip a`). IP đổi mỗi khi đổi mạng nên phải sửa lại.
3. **Nạp firmware:** bấm Upload trong PlatformIO (hoặc `pio run -t upload`), xem log bằng `pio device monitor`. Thấy `[net] mqtt connected` là thông.
4. **Kiểm tra:** `mosquitto_sub -h <IP PC> -t "smartroom/#" -v` sẽ thấy telemetry mỗi giây. Gửi thử lệnh:
   `mosquitto_pub -h <IP PC> -t smartroom/nodeA/cmd -m '{"command_id":"t1","target":"fan","action":"ON"}'`
5. **Gateway lên CoreIOT:** xem mục "Gateway lên CoreIOT" bên dưới.

## Nạp firmware (chi tiết)
1. Cài VS Code + extension PlatformIO. Mở thư mục này.
2. Sửa `include/secrets.h` (Wi-Fi, IP máy PC chạy Mosquitto) và `include/pins.h`.
3. Cắm board, bấm **Upload** (mũi tên → ở thanh dưới) hoặc `pio run -t upload`. Xem log: `pio device monitor`.
   Nếu không nạp được: giữ **BOOT**, bấm **RESET**, thả BOOT rồi nạp lại.

## Test trên PC
`sh host_tests/run_tests.sh` (cần gcc và python3).

## Gateway lên CoreIOT
Xem `gateway/README.md` trong repo gateway/CoreIOT (TV3). Topic thiết bị mặc định: `smartroom/nodeA/...`.
