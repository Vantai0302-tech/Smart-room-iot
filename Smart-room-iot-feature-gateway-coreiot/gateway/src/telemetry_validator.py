
import math


class TelemetryValidator:
    def __init__(self):
        self.last_seq = {}

    def validate(self, data):
        if not isinstance(data, dict):
            return False, "INVALID_FORMAT"

        required = (
            "device_id", "seq", "temperature_c",
            "humidity_pct", "light_lux",
            "door_open", "sensor_valid"
        )

        if any(key not in data for key in required):
            return False, "MISSING_FIELD"

        device_id = data["device_id"]
        seq = data["seq"]

        if not isinstance(device_id, str) or not device_id.strip():
            return False, "INVALID_DEVICE_ID"

        if type(seq) is not int or seq < 0:
            return False, "INVALID_SEQUENCE"

        for key in ("door_open", "sensor_valid"):
            if type(data[key]) is not bool:
                return False, f"INVALID_TYPE_{key}"

        for key in ("temperature_c", "humidity_pct", "light_lux"):
            value = data[key]

            if type(value) not in (int, float):
                return False, f"INVALID_TYPE_{key}"

            if not math.isfinite(value):
                return False, f"NONFINITE_{key}"

        if not -40 <= data["temperature_c"] <= 125:
            return False, "INVALID_TEMPERATURE"

        if not 0 <= data["humidity_pct"] <= 100:
            return False, "INVALID_HUMIDITY"

        if not 0 <= data["light_lux"] <= 65535:
            return False, "INVALID_LIGHT"

        if not data["sensor_valid"]:
            return False, "SENSOR_ERROR"

        previous = self.last_seq.get(device_id)

        if previous is not None and seq <= previous:
            return False, "DUPLICATE_OR_OLD_SEQUENCE"

        self.last_seq[device_id] = seq
        return True, "VALID"
