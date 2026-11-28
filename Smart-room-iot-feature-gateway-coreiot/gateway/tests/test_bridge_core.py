import json
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

import bridge_core as bc
from telemetry_validator import TelemetryValidator


class Fake:
    def __init__(self):
        self.sent = []

    def publish(self, topic, payload, qos=1, retain=False):
        self.sent.append((topic, json.loads(payload), qos, retain))


def make():
    loc, cld = Fake(), Fake()
    events, logs = [], []
    b = bc.Bridge("nodeA", loc, cld, validator=TelemetryValidator(), clock=lambda: 1_000_000,
                  on_event=lambda k, d: events.append((k, d)), log=logs.append)
    return b, loc, cld, events, logs


def telemetry(seq=1, valid=True, temp=33.0):
    return json.dumps({"device_id": "nodeA", "seq": seq, "temperature_c": temp, "humidity_pct": 55.5,
                       "light_lux": 200.0, "door_open": False, "sample_time_ms": 1234,
                       "sensor_valid": valid}).encode()


class T(unittest.TestCase):
    def test_rpc_mapping(self):
        c = bc.rpc_to_command("setFan", False, 5, new_id=lambda: "x")
        self.assertEqual((c["target"], c["action"], c["expires_at_ms"]), ("fan", "OFF", 5 + bc.COMMAND_TTL_MS))
        c = bc.rpc_to_command("setLight", {"value": True, "duration_ms": 60000}, 5)
        self.assertEqual((c["action"], c["duration_ms"]), ("ON", 60000))
        for bad in [("setHeater", True), ("setFan", "ON"), ("setFan", 1), ("setFan", {"value": True, "duration_ms": -1}),
                    ("setFan", {"value": True, "duration_ms": True}), ("setFan", None)]:
            self.assertIsNone(bc.rpc_to_command(*bad, 5), bad)

    def test_valid_telemetry_forwarded_with_tv3_keys(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/telemetry", telemetry())
        topics_sent = [(t, p) for t, p, _, _ in cld.sent]
        self.assertIn(("v1/devices/me/telemetry",
                       {"temperature_c": 33.0, "humidity_pct": 55.5, "light_lux": 200.0, "door_open": False}), topics_sent)
        self.assertIn(("v1/devices/me/attributes", {"sensor_ok": True}), topics_sent)
        self.assertTrue(any("[VALID]" in l for l in logs))

    def test_invalid_telemetry_not_forwarded(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/telemetry", telemetry(temp=999))
        self.assertEqual(cld.sent, [])
        self.assertIn("[REJECTED] INVALID_TEMPERATURE", logs)
        b.on_local("smartroom/nodeA/telemetry", b"not json")      # không được văng lỗi
        self.assertTrue(any("[ERROR]" in l for l in logs))

    def test_duplicate_seq_rejected_but_seq_after_reboot_ok(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/telemetry", telemetry(seq=1000001))
        b.on_local("smartroom/nodeA/telemetry", telemetry(seq=1000001))    # trùng
        self.assertIn("[REJECTED] DUPLICATE_OR_OLD_SEQUENCE", logs)
        b.on_local("smartroom/nodeA/telemetry", telemetry(seq=2000001))    # boot lần 2: seq nhảy lên
        self.assertEqual(sum(1 for t, *_ in cld.sent if t == "v1/devices/me/telemetry"), 2)

    def test_sensor_error_reported_once(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/telemetry", telemetry(seq=1, valid=False))
        b.on_local("smartroom/nodeA/telemetry", telemetry(seq=2, valid=False))
        flags = [p for t, p, *_ in cld.sent if t == "v1/devices/me/attributes"]
        self.assertEqual(flags, [{"sensor_ok": False}])             # chỉ báo khi đổi trạng thái
        self.assertFalse(any(t == "v1/devices/me/telemetry" for t, *_ in cld.sent))

    def test_end_to_end_rpc_ack(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/status", b"online")
        b.on_cloud("v1/devices/me/rpc/request/77", json.dumps({"method": "setFan", "params": False}).encode())
        topic, cmd, qos, retain = loc.sent[-1]
        self.assertEqual((topic, qos, retain), ("smartroom/nodeA/cmd", 1, False))
        self.assertEqual((cmd["target"], cmd["action"], cmd["source"]), ("fan", "OFF", "cloud"))
        ack = {"command_id": cmd["command_id"], "status": "APPLIED", "reason_code": "OK",
               "actual_state": {"fan": False, "light": False}, "state_version": 8, "mode": "STUDY", "override_remaining_ms": 60000}
        b.on_local("smartroom/nodeA/ack", json.dumps(ack).encode())
        self.assertIn(("v1/devices/me/rpc/response/77", {"status": "APPLIED", "reason_code": "OK"}, 1, False), cld.sent)
        self.assertEqual(b.pending, {})

    def test_offline_device_rejected_not_queued(self):
        b, loc, cld, ev, logs = make()
        b.on_cloud("v1/devices/me/rpc/request/5", json.dumps({"method": "setFan", "params": True}).encode())
        self.assertEqual(loc.sent, [])
        self.assertEqual(cld.sent[-1], ("v1/devices/me/rpc/response/5", {"status": "REJECTED", "reason_code": "DEVICE_OFFLINE"}, 1, False))

    def test_bad_rpc_and_timeout(self):
        b, loc, cld, ev, logs = make()
        b.on_local("smartroom/nodeA/status", b"online")
        b.on_cloud("v1/devices/me/rpc/request/9", b"garbage")
        self.assertEqual(cld.sent[-1][1]["reason_code"], "BAD_RPC")
        b.on_cloud("v1/devices/me/rpc/request/10", json.dumps({"method": "setFan", "params": True}).encode())
        cid = next(iter(b.pending))
        b.pending[cid] = ("10", 0)
        b.sweep()
        self.assertEqual(cld.sent[-1], ("v1/devices/me/rpc/response/10", {"status": "REJECTED", "reason_code": "TIMEOUT"}, 1, False))


class TestCoreIOTClient(unittest.TestCase):
    def test_requires_token_and_dispatches_rpc(self):
        import coreiot_client as cc
        os.environ.pop("COREIOT_ACCESS_TOKEN", None)
        with self.assertRaises(ValueError):
            cc.CoreIOTClient()
        os.environ["COREIOT_ACCESS_TOKEN"] = "dummy-test-token"
        c = cc.CoreIOTClient()
        got = []
        c.on_rpc_request = lambda t, p: got.append((t, p))

        class M: topic, payload = "v1/devices/me/rpc/request/1", b"{}"
        c.on_message(None, None, M)
        self.assertEqual(got, [("v1/devices/me/rpc/request/1", b"{}")])
        self.assertFalse(c.publish("x", "y"))          # chưa kết nối: bỏ và báo False


if __name__ == "__main__":
    unittest.main(verbosity=1)
