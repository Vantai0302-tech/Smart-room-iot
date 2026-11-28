
import json
import time
import paho.mqtt.client as mqtt

BROKER = "127.0.0.1"
PORT = 1883
TOPIC = "smartroom/nodeA/telemetry"

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="smartroom-mock-device"
)

client.connect(BROKER, PORT, 60)
client.loop_start()

try:
    sequence = 0

    while True:
        sequence += 1

        sensor_data = {
            "device_id": "yolouno_01",
            "seq": sequence,
            "temperature_c": 33.5,
            "humidity_pct": 65.0,
            "light_lux": 120.0,
            "door_open": False,
            "sensor_valid": True
        }

        payload = json.dumps(sensor_data)

        message = client.publish(
            TOPIC,
            payload,
            qos=1
        )
        message.wait_for_publish()

        print(f"[PUBLISH] {payload}")

        time.sleep(2)

except KeyboardInterrupt:
    print("\nStopping mock device...")

finally:
    client.disconnect()
    client.loop_stop()
