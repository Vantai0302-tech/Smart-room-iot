"""Lõi cầu nối Gateway: dịch bản tin giữa thiết bị (MQTT cục bộ) và CoreIOT (ThingsBoard MQTT).
Dùng TelemetryValidator của TV3 để kiểm tra telemetry. Không import thư viện mạng nên test được ngay.
Topic thiết bị: smartroom/<device_id>/{telemetry,state,ack,cmd,status}.
"""
import json
import time
import uuid

LOCAL = "smartroom"
TB_TELEMETRY = "v1/devices/me/telemetry"
TB_ATTRIBUTES = "v1/devices/me/attributes"
TB_RPC_REQ = "v1/devices/me/rpc/request/"
TB_RPC_RESP = "v1/devices/me/rpc/response/"

RPC_TARGETS = {"setFan": "fan", "setLight": "light"}   # TV3 đối chiếu với widget dashboard
COMMAND_TTL_MS = 10_000      # lệnh quá 10 giây là lệnh cũ
RPC_TIMEOUT_S = 10.0         # chờ ACK từ thiết bị tối đa 10 giây
TELEMETRY_KEYS = ("temperature_c", "humidity_pct", "light_lux", "door_open")


def now_ms():
    return int(time.time() * 1000)


def topics(device_id):
    return {k: f"{LOCAL}/{device_id}/{k}" for k in ("telemetry", "state", "ack", "cmd", "status")}


def rpc_to_command(method, params, now, new_id=lambda: "gw-" + uuid.uuid4().hex[:12]):
    """RPC của CoreIOT -> lệnh gửi thiết bị. Trả về None nếu method/params không hợp lệ."""
    target = RPC_TARGETS.get(method)
    if target is None:
        return None
    duration = 0
    if isinstance(params, dict):
        duration = params.get("duration_ms", 0)
        params = params.get("value")
    if not isinstance(params, bool) or not isinstance(duration, int) or isinstance(duration, bool) or duration < 0:
        return None
    return {"command_id": new_id(), "target": target, "action": "ON" if params else "OFF",
            "source": "cloud", "duration_ms": duration,
            "issued_at_ms": now, "expires_at_ms": now + COMMAND_TTL_MS, "policy_version": 1}


def telemetry_to_cloud(data):
    """Lấy các khóa telemetry (cùng bộ khóa TV3 đã dùng thử) từ bản tin đã được kiểm tra."""
    return {k: data[k] for k in TELEMETRY_KEYS if k in data}


def state_to_cloud(d):
    """state/ack của thiết bị -> thuộc tính (actual state) trên CoreIOT."""
    actual = d.get("actual_state", {})
    return {"fan": actual.get("fan"), "light": actual.get("light"), "mode": d.get("mode"),
            "state_version": d.get("state_version"), "override_remaining_ms": d.get("override_remaining_ms")}


class Bridge:
    """Nối hai phía. `local` và `cloud` chỉ cần có publish(topic, payload, qos=1, retain=False)."""

    def __init__(self, device_id, local, cloud, validator=None, clock=now_ms, on_event=None, log=print):
        self.t = topics(device_id)
        self.local, self.cloud, self.clock, self.log = local, cloud, clock, log
        self.validator = validator
        self.on_event = on_event or (lambda kind, data: None)   # TV3 gắn SQLite tại đây
        self.pending = {}          # command_id -> (rpc_request_id, deadline_s)
        self.device_online = False
        self._sensor_ok = None

    # ---- phía thiết bị -> cloud ----
    def on_local(self, topic, payload):
        try:
            self._on_local(topic, payload)
        except (ValueError, KeyError, AttributeError, TypeError) as error:
            self.log(f"[ERROR] Bad message on {topic}: {error}")

    def _on_local(self, topic, payload):
        if topic == self.t["telemetry"]:
            data = json.loads(payload)
            ok, reason = self.validator.validate(data) if self.validator else (True, "VALID")
            if ok:
                self._set_sensor_ok(True)
                self.cloud.publish(TB_TELEMETRY, json.dumps(telemetry_to_cloud(data)))
                self.log(f"[VALID] Device={data.get('device_id')} Seq={data.get('seq')} "
                         f"Temp={data.get('temperature_c')} C Humidity={data.get('humidity_pct')}%")
                self.on_event("telemetry", data)
            else:
                if reason == "SENSOR_ERROR":    # báo CoreIOT để RuleChain/dashboard cảnh báo cảm biến lỗi
                    self._set_sensor_ok(False)
                self.log(f"[REJECTED] {reason}")
        elif topic == self.t["state"]:
            d = json.loads(payload)
            self.cloud.publish(TB_ATTRIBUTES, json.dumps(state_to_cloud(d)))
            self.on_event("state", d)
        elif topic == self.t["ack"]:
            ack = json.loads(payload)
            self.cloud.publish(TB_ATTRIBUTES, json.dumps(state_to_cloud(ack)))
            self.on_event("ack", ack)
            req = self.pending.pop(ack.get("command_id"), None)
            if req:
                self.cloud.publish(TB_RPC_RESP + req[0], json.dumps(
                    {"status": ack.get("status"), "reason_code": ack.get("reason_code")}))
        elif topic == self.t["status"]:
            text = payload.decode() if isinstance(payload, bytes) else payload
            self.device_online = text == "online"
            self.cloud.publish(TB_ATTRIBUTES, json.dumps({"device_online": self.device_online}))
            self.on_event("status", {"online": self.device_online})

    def _set_sensor_ok(self, ok):
        if ok != self._sensor_ok:       # chỉ báo khi đổi trạng thái
            self._sensor_ok = ok
            self.cloud.publish(TB_ATTRIBUTES, json.dumps({"sensor_ok": ok}))

    # ---- cloud -> thiết bị ----
    def on_cloud(self, topic, payload):
        if not topic.startswith(TB_RPC_REQ):
            return
        req_id = topic[len(TB_RPC_REQ):]
        try:
            msg = json.loads(payload)
            cmd = rpc_to_command(msg.get("method"), msg.get("params"), self.clock())
        except (ValueError, AttributeError):
            cmd = None
        if cmd is None:
            return self._reply(req_id, "REJECTED", "BAD_RPC")
        if not self.device_online:       # không xếp hàng lệnh cho thiết bị offline: tránh lệnh cũ bị áp lại
            self.on_event("rpc_rejected", {"command": cmd, "reason": "DEVICE_OFFLINE"})
            return self._reply(req_id, "REJECTED", "DEVICE_OFFLINE")
        self.pending[cmd["command_id"]] = (req_id, time.monotonic() + RPC_TIMEOUT_S)
        self.local.publish(self.t["cmd"], json.dumps(cmd), qos=1, retain=False)   # không retain
        self.on_event("command_sent", cmd)

    def sweep(self):
        """Gọi định kỳ: trả lời RPC quá hạn bằng TIMEOUT."""
        now = time.monotonic()
        for cid, (req_id, deadline) in list(self.pending.items()):
            if now >= deadline:
                del self.pending[cid]
                self._reply(req_id, "REJECTED", "TIMEOUT")
                self.on_event("rpc_timeout", {"command_id": cid})

    def _reply(self, req_id, status, reason):
        self.cloud.publish(TB_RPC_RESP + req_id, json.dumps({"status": status, "reason_code": reason}))
