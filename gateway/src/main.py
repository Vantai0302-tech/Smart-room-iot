
import json
import paho.mqtt.client as mqtt
from telemetry_validator import TelemetryValidator

validator = TelemetryValidator()

BROKER = "127.0.0.1"
PORT = 1883
TOPIC = "smartroom/nodeA/telemetry"


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("[GATEWAY] Connected to MQTT Broker")
        client.subscribe(TOPIC, qos=1)
        print(f"[SUBSCRIBE] {TOPIC}")
    else:
        print(f"[ERROR] Connection failed: {reason_code}")


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode("utf-8"))

        valid, reason = validator.validate(data)

        if not valid:
            print(f"[REJECTED] {reason}")
            return

        print(
            f"[VALID] Device={data['device_id']} "
            f"Seq={data['seq']} "
            f"Temp={data['temperature_c']} C "
            f"Humidity={data['humidity_pct']}%"
        )

    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        print(f"[ERROR] Invalid JSON: {error}")


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="smartroom-gateway"
)

client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT, 60)
    client.loop_forever()
except KeyboardInterrupt:
    print("\n[GATEWAY] Stopping...")
finally:
    client.disconnect()
