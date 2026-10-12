"""Enrollment by devices nobody has approved is bounded, and approval makes a record permanent."""

from __future__ import annotations

import base64
import json
import secrets

import pytest

from hermes_gadget_plugin import hub as hubmod
from hermes_gadget_plugin import protocol


def _enroll(url: str, name: str) -> tuple[str, dict]:
    """Connect as a brand-new device; returns (device_id, the frame after auth)."""
    from websockets.sync.client import connect

    key = secrets.token_bytes(32)
    device_id = protocol.device_id_for_key(key)
    with connect(url, subprotocols=[protocol.SUBPROTOCOL], open_timeout=5) as ws:
        ws.send(json.dumps(protocol.message("hello", proto=protocol.VERSION, device_id=device_id, name=name,
                                            board="sim-320x240", caps={})))
        challenge = json.loads(ws.recv(timeout=5))
        assert challenge["type"] == "challenge" and challenge["enrolled"] is False
        ws.send(json.dumps(protocol.message("auth", key=base64.b64encode(key).decode())))
        answer = json.loads(ws.recv(timeout=5))
    return device_id, answer


def test_unapproved_enrollments_are_capped_per_address(devserver, monkeypatch):
    monkeypatch.setattr(hubmod, "MAX_PENDING_PER_ADDRESS", 2)
    hub, brain, url = devserver(require_pairing=True)

    first, welcome = _enroll(url, "one")
    assert welcome["type"] == "welcome" and welcome["paired"] is False
    second, _ = _enroll(url, "two")
    assert set(hub.store.pending()) == {first, second}
    assert all(rec["address"] == "127.0.0.1" for rec in hub.store.pending().values())

    third, refused = _enroll(url, "three")
    assert refused["type"] == "error" and refused["code"] == "busy"
    assert "127.0.0.1" in refused["message"]
    assert third not in hub.store.devices(), "a refused device leaves nothing behind"

    # An approved device stops counting, so the next stranger gets in.
    brain.approved.add(first)
    _, again = _enroll(url, "one again")  # a new key: the old one is still pending
    assert again["type"] == "error"
    hub.store.confirm(first)
    assert set(hub.store.pending()) == {second}
    fourth, welcome = _enroll(url, "four")
    assert welcome["type"] == "welcome"
    assert set(hub.store.pending()) == {second, fourth}


def test_the_total_cap_applies_across_addresses(devserver, monkeypatch):
    monkeypatch.setattr(hubmod, "MAX_PENDING_DEVICES", 1)
    monkeypatch.setattr(hubmod, "MAX_PENDING_PER_ADDRESS", 100)
    hub, brain, url = devserver(require_pairing=True)
    _enroll(url, "one")
    _, refused = _enroll(url, "two")
    assert refused["type"] == "error" and refused["code"] == "busy"
    assert "1 devices are already waiting" in refused["message"]


def test_a_device_admitted_without_pairing_is_never_pending(devserver):
    hub, brain, url = devserver(require_pairing=False)
    device_id, welcome = _enroll(url, "open house")
    assert welcome["paired"] is True
    assert hub.store.pending() == {}
    assert hub.store.devices()[device_id]["pending"] is False


def test_approved_new_device_bypasses_pending_limit(devserver, monkeypatch):
    monkeypatch.setattr(hubmod, "MAX_PENDING_DEVICES", 0)
    hub, brain, url = devserver(require_pairing=False)
    device_id, welcome = _enroll(url, "allowed")
    assert welcome["type"] == "welcome" and welcome["paired"] is True
    assert device_id in hub.store.devices()


def test_live_pending_session_refreshes_activity_without_writing_every_frame(tmp_path, monkeypatch):
    import asyncio
    from hermes_gadget_plugin.store import DeviceStore, PENDING_TTL_S

    now = [1000.0]
    monkeypatch.setattr(hubmod.time, "time", lambda: now[0])
    store = DeviceStore(tmp_path)
    store.enroll("device", b"k" * 32)
    writes = []
    touch = store.touch
    monkeypatch.setattr(store, "touch", lambda *a, **kw: (writes.append(now[0]), touch(*a, **kw))[-1])

    class Socket:
        async def send(self, _):
            pass
        def __aiter__(self):
            return self.frames()
        async def frames(self):
            for _ in range(PENDING_TTL_S // 10 + 10):
                now[0] += 10
                yield '{"type":"pong"}'
                assert store.key_for("device") is not None

    class Unpaired(hubmod.HubDelegate):
        async def is_paired(self, _):
            return False
    hub = hubmod.DeviceHub(store, Unpaired())
    async def authenticated(_):
        return {"device_id": "device"}
    monkeypatch.setattr(hub, "_authenticate", authenticated)
    asyncio.run(hub._handle(Socket()))
    assert 0 < len(writes) < 100
    assert store.key_for("device") is not None


def test_approval_over_the_connection_confirms_the_record(devserver, loop_thread):
    hub, brain, url = devserver(require_pairing=True)
    device_id, welcome = _enroll(url, "soon approved")
    assert welcome["paired"] is False
    assert device_id in hub.store.pending()

    # The approval path marks the session paired; the store keeps the record for good.
    from hermes_gadget_plugin.hub import DeviceSession

    class Quiet:
        async def send(self, _):
            pass
        remote_address = ("127.0.0.1", 1)

    session = DeviceSession(hub, Quiet(), {"device_id": device_id})
    loop_thread.run(session.set_paired(True))
    assert device_id not in hub.store.pending()
    assert hub.store.devices()[device_id]["pending"] is False
