
import json
import queue
import threading

import paho.mqtt.client as mqtt

from telemetry_validator import TelemetryValidator
from coreiot_client import CoreIOTClient


# Local MQTT configuration
LOCAL_BROKER = "127.0.0.1"
LOCAL_PORT = 1883
TELEMETRY_TOPIC = "smartroom/nodeA/telemetry"

# Shared modules
validator = TelemetryValidator()
telemetry_queue = queue.Queue(maxsize=100)


# Called after connecting to Local Mosquitto
def on_connect(client, userdata, flags,
               reason_code, properties):

    if reason_code == 0:
        print("[LOCAL] Connected")

        client.subscribe(
            TELEMETRY_TOPIC,
            qos=1
        )

        print(f"[SUBSCRIBE] {TELEMETRY_TOPIC}")

    else:
        print(f"[LOCAL ERROR] {reason_code}")


# Called when telemetry arrives from Mock Device
def on_message(client, userdata, msg):

    try:
        data = json.loads(
            msg.payload.decode("utf-8")
        )

        valid, reason = validator.validate(data)

        if not valid:
            print(f"[REJECTED] {reason}")
            return

        try:
            telemetry_queue.put_nowait(data)

        except queue.Full:
            print("[WARNING] Telemetry queue full")
            return

        print(
            f"[LOCAL RX] "
            f"Device={data['device_id']} "
            f"Seq={data['seq']} "
            f"Temp={data['temperature_c']}"
        )

    except (UnicodeDecodeError,
            json.JSONDecodeError) as error:

        print(f"[JSON ERROR] {error}")


# Worker sends validated telemetry to CoreIOT
def cloud_worker(cloud, stop_event):

    while not stop_event.is_set():

        try:
            data = telemetry_queue.get(timeout=0.5)

        except queue.Empty:
            continue

        try:
            while not stop_event.is_set():

                try:
                    cloud.publish_telemetry(data)

                    print(
                        f"[BRIDGE OK] "
                        f"Seq={data['seq']}"
                    )

                    break

                except (ConnectionError,
                        TimeoutError,
                        RuntimeError,
                        OSError) as error:

                    print(f"[CLOUD ERROR] {error}")
                    stop_event.wait(3)

        finally:
            telemetry_queue.task_done()


def main():

    stop_event = threading.Event()

    local_client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="smartroom-gateway-local"
    )

    local_client.on_connect = on_connect
    local_client.on_message = on_message

    cloud = None
    worker = None

    try:
        # Connect Cloud first
        cloud = CoreIOTClient()
        cloud.connect()

        # Start background Cloud publisher
        worker = threading.Thread(
            target=cloud_worker,
            args=(cloud, stop_event),
            daemon=True
        )

        worker.start()

        # Connect Local Broker
        local_client.connect(
            LOCAL_BROKER,
            LOCAL_PORT,
            keepalive=60
        )

        print("[GATEWAY] Bridge started")

        local_client.loop_forever()

    except KeyboardInterrupt:
        print("\n[GATEWAY] Stopping...")

    except (OSError, ValueError,
            TimeoutError) as error:

        print(f"[GATEWAY ERROR] {error}")

    finally:
        stop_event.set()
        local_client.disconnect()

        if worker is not None:
            worker.join(timeout=1)

        if cloud is not None:
            cloud.close()


if __name__ == "__main__":
    main()
