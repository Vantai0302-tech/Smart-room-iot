"""Gateway PC: board (MQTT cục bộ) <-> CoreIOT.
Chạy:  cd gateway/src && python main.py
Biến môi trường: COREIOT_ACCESS_TOKEN (bắt buộc) | DEVICE_ID (nodeA) | LOCAL_HOST (127.0.0.1) | LOCAL_PORT (1883)
                 COREIOT_HOST, COREIOT_PORT (mặc định theo coreiot_client.py)
"""
import os
import time
import traceback
import paho.mqtt.client as mqtt

from bridge_core import Bridge, topics
from coreiot_client import CoreIOTClient
from telemetry_validator import TelemetryValidator

DEVICE_ID = os.getenv("DEVICE_ID", "nodeA")
LOCAL_HOST = os.getenv("LOCAL_HOST", "127.0.0.1")
LOCAL_PORT = int(os.getenv("LOCAL_PORT", "1883"))


class _LocalPub:
    def __init__(self, client):
        self.client = client

    def publish(self, topic, payload, qos=1, retain=False):
        self.client.publish(topic, payload, qos=qos, retain=retain)


def guarded(fn, *args):
    """Lỗi bất ngờ trong một bản tin không được làm chết luồng MQTT."""
    try:
        fn(*args)
    except Exception:
        traceback.print_exc()


def main():
    t = topics(DEVICE_ID)

    local = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="smartroom-gateway")
    cloud = CoreIOTClient()                     # thiếu token sẽ báo lỗi rõ ràng ngay
    bridge = Bridge(DEVICE_ID, _LocalPub(local), cloud, validator=TelemetryValidator())

    def on_local_connect(client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            print("[GATEWAY] Connected to local MQTT broker")
            for key in ("telemetry", "state", "ack", "status"):
                client.subscribe(t[key], qos=1)
                print(f"[SUBSCRIBE] {t[key]}")
        else:
            print(f"[ERROR] Local connection failed: {reason_code}")

    local.on_connect = on_local_connect
    local.on_message = lambda c, u, m: guarded(bridge.on_local, m.topic, m.payload)
    cloud.on_rpc_request = lambda topic, payload: guarded(bridge.on_cloud, topic, payload)

    local.connect_async(LOCAL_HOST, LOCAL_PORT, 60)   # chưa bật broker thì tự thử lại
    local.loop_start()
    try:
        cloud.connect()
    except TimeoutError as error:
        print(f"[WARN] {error}: tiếp tục chạy, sẽ tự nối lại CoreIOT")

    try:
        while True:
            bridge.sweep()
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[GATEWAY] Stopping...")
    finally:
        local.loop_stop()
        local.disconnect()
        cloud.close()


if __name__ == "__main__":
    main()
