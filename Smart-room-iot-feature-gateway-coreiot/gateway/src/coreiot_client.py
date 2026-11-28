
import os
import json
import threading
import paho.mqtt.client as mqtt

# Địa chỉ CoreIOT: giữ mặc định của TV3, có thể đổi bằng biến môi trường (không sửa code)
BROKER = os.getenv("COREIOT_HOST", "app.coreiot.io")
PORT = int(os.getenv("COREIOT_PORT", "1883"))
TELEMETRY_TOPIC = "v1/devices/me/telemetry"
RPC_REQUEST_FILTER = "v1/devices/me/rpc/request/+"


class CoreIOTClient:
    def __init__(self):
        token = os.getenv("COREIOT_ACCESS_TOKEN")

        if not token:
            raise ValueError("Missing COREIOT_ACCESS_TOKEN")

        self.connected = threading.Event()
        self.on_rpc_request = None      # hàm(topic, payload) do gateway gắn vào

        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="smartroom-gateway-cloud"
        )

        self.client.username_pw_set(token)
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.reconnect_delay_set(min_delay=1, max_delay=30)

    def on_connect(self, client, userdata, flags,
                   reason_code, properties):
        if reason_code == 0:
            self.connected.set()
            # Đăng ký lại mỗi lần nối (kể cả sau khi tự nối lại)
            client.subscribe(RPC_REQUEST_FILTER, qos=1)
            print("[COREIOT] Connected")
        else:
            self.connected.clear()
            print(f"[COREIOT] Connection failed: {reason_code}")

    def on_disconnect(self, client, userdata, flags,
                      reason_code, properties):
        self.connected.clear()
        print(f"[COREIOT] Disconnected: {reason_code}")

    def on_message(self, client, userdata, msg):
        if self.on_rpc_request is not None:
            self.on_rpc_request(msg.topic, msg.payload)

    def connect(self):
        # connect_async: nếu mạng chưa sẵn sàng thì vẫn tự thử lại ngầm
        self.client.connect_async(BROKER, PORT, keepalive=60)
        self.client.loop_start()

        if not self.connected.wait(timeout=10):
            raise TimeoutError("CoreIOT connection timeout")

    def publish(self, topic, payload, qos=1, retain=False):
        """Gửi không chờ (dùng cho gateway). Mất kết nối thì bỏ và báo False."""
        if not self.connected.is_set():
            print("[COREIOT] Offline, dropped message")
            return False

        result = self.client.publish(topic, payload, qos=qos, retain=retain)
        return result.rc == mqtt.MQTT_ERR_SUCCESS

    def publish_telemetry(self, data):
        if not self.connected.is_set():
            raise ConnectionError("CoreIOT is disconnected")

        payload = json.dumps(data)

        result = self.client.publish(
            TELEMETRY_TOPIC,
            payload,
            qos=1
        )

        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            raise RuntimeError(f"Publish error: {result.rc}")

        result.wait_for_publish(timeout=10)

        if not result.is_published():
            raise TimeoutError("Telemetry PUBACK timeout")

        print(f"[COREIOT] Published: {payload}")

    def close(self):
        self.client.disconnect()
        self.client.loop_stop()


if __name__ == "__main__":
    cloud = CoreIOTClient()

    try:
        cloud.connect()

        telemetry = {
            "temperature_c": 33.5,
            "humidity_pct": 65.0,
            "light_lux": 120.0,
            "door_open": False
        }

        cloud.publish_telemetry(telemetry)

    except (OSError, ConnectionError, TimeoutError,
            RuntimeError) as error:
        print(f"[ERROR] {error}")

    finally:
        cloud.close()
