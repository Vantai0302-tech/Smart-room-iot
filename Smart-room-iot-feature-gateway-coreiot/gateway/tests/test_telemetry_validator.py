
import sys
import unittest
from pathlib import Path

sys.path.insert(
    0, str(Path(__file__).resolve().parents[1] / "src")
)

from telemetry_validator import TelemetryValidator


class TestTelemetryValidator(unittest.TestCase):

    def setUp(self):
        self.validator = TelemetryValidator()
        self.data = {
            "device_id": "yolouno_01",
            "seq": 1,
            "temperature_c": 33.5,
            "humidity_pct": 65.0,
            "light_lux": 120.0,
            "door_open": False,
            "sensor_valid": True
        }

    def test_valid(self):
        valid, reason = self.validator.validate(self.data)
        self.assertTrue(valid)
        self.assertEqual(reason, "VALID")

    def test_invalid_humidity(self):
        self.data["humidity_pct"] = 150
        valid, reason = self.validator.validate(self.data)
        self.assertFalse(valid)
        self.assertEqual(reason, "INVALID_HUMIDITY")

    def test_sensor_error(self):
        self.data["sensor_valid"] = False
        valid, reason = self.validator.validate(self.data)
        self.assertFalse(valid)
        self.assertEqual(reason, "SENSOR_ERROR")

    def test_duplicate_sequence(self):
        self.validator.validate(self.data)
        valid, reason = self.validator.validate(self.data)
        self.assertFalse(valid)
        self.assertEqual(reason, "DUPLICATE_OR_OLD_SEQUENCE")

    def test_invalid_temperature_type(self):
        self.data["temperature_c"] = "ERROR"
        valid, reason = self.validator.validate(self.data)
        self.assertFalse(valid)
        self.assertEqual(reason, "INVALID_TYPE_temperature_c")


if __name__ == "__main__":
    unittest.main()
