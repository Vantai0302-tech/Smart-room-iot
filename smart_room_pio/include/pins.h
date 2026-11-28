#pragma once
/* CHÂN GPIO: số dưới đây là GIÁ TRỊ TẠM, hãy đối chiếu sơ đồ chân YoloUNO của bạn rồi sửa. */
#define PIN_I2C_SDA    11   /* BH1750 */
#define PIN_I2C_SCL    12
#define PIN_DOOR        6   /* công tắc từ: kéo lên, cửa đóng = mức thấp */
#define PIN_BUTTON      0   /* nút nhấn: kéo lên, nhấn = mức thấp */
#define PIN_RS485_TX   17   /* SHT20 qua RS485 */
#define PIN_RS485_RX   18
#define PIN_RS485_DE    8   /* chân DE/RE của module RS485 (nếu module tự đảo chiều thì bỏ qua) */
#define RS485_BAUD   9600

/* Cảm biến nhiệt độ/ẩm: 1 = DHT20 (I2C, địa chỉ 0x38, chung bus với BH1750)
                         0 = SHT20 Modbus RS485 (yêu cầu T02 của đề) */
#define USE_DHT20 1

/* 1 = KHÔNG cần cảm biến: nhiệt độ/ẩm/ánh sáng là số giả lập (nhiệt độ chạy 25 -> 36,5 °C mỗi 24 giây,
       10 giây cuối mỗi phút "mất cảm biến" để thấy sensor_valid=0). Cửa và nút vẫn đọc chân thật.
   0 = dùng cảm biến thật (DHT20 hoặc SHT20 theo USE_DHT20 ở trên). */
#define USE_MOCK_SENSORS 1
