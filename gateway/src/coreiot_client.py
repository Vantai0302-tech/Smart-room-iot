import time
import os
import json
import threading
import paho.mqtt.client as mqtt

BROKER = "app.coreiot.io"
PORT = 1883
TELEMETRY_TOPIC = "v1/devices/me/telemetry"


class CoreIOTClient:
    def __init__(self):
        token = os.getenv("COREIOT_ACCESS_TOKEN")

        if not token:
            raise ValueError("Missing COREIOT_ACCESS_TOKEN")

        self.connected = threading.Event()

        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="smartroom-gateway-cloud"
        )

        self.client.username_pw_set(token)
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect

    def on_connect(self, client, userdata, flags,
                   reason_code, properties):
        if reason_code == 0:
            self.connected.set()
            print("[COREIOT] Connected")
        else:
            self.connected.clear()
            print(f"[COREIOT] Connection failed: {reason_code}")

    def on_disconnect(self, client, userdata, flags,
                      reason_code, properties):
        self.connected.clear()
        print(f"[COREIOT] Disconnected: {reason_code}")

    def connect(self):
        self.client.connect(BROKER, PORT, keepalive=60)
        self.client.loop_start()

        if not self.connected.wait(timeout=10):
            raise TimeoutError("CoreIOT connection timeout")

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
        start = time.monotonic()

        result.wait_for_publish(timeout=30)

        elapsed = time.monotonic() - start

        if not result.is_published():
            print(
                f"[CLOUD TIMEOUT] "
                f"Seq={data.get('seq')} "
                f"MID={result.mid} "
                f"Wait={elapsed:.2f}s"
                )
            raise TimeoutError("Telemetry PUBACK timeout")
        
        print(
            f"[CLOUD ACK] "
            f"Seq={data.get('seq')} "
            f"MID={result.mid} "
            f"Latency={elapsed:.2f}s"
            )

        

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
