"""The setup step and `hermes gadget pair` against Hermes's real config writer and pairing store,
in a temporary Hermes home.

Run with a Python that can import Hermes, e.g.:
    HERMES_AGENT_DIR=../hermes-agent ../hermes-agent/.venv/Scripts/python -m pytest tests/test_setup_hermes.py
"""

from __future__ import annotations

import types

import pytest

from conftest import requires_hermes

pytestmark = requires_hermes

DEVICE = "hg-0123456789abcdef"


@pytest.fixture
def hermes_home(tmp_path, monkeypatch):
    home = tmp_path / "hermes-home"
    home.mkdir()
    monkeypatch.setenv("HERMES_HOME", str(home))
    return home


def test_setup_enables_the_platform_in_config_yaml(hermes_home, monkeypatch):
    from hermes_cli.config import get_config_path, load_config

    from hermes_gadget_plugin import setup

    monkeypatch.setattr("hermes_cli.setup.prompt", lambda question, default=None, password=False: "9100")
    setup.interactive_setup()
    assert get_config_path() == hermes_home / "config.yaml" and get_config_path().is_file()
    gadget = load_config()["platforms"]["gadget"]
    assert gadget["enabled"] is True
    assert gadget["extra"]["port"] == 9100
    assert load_config()["tts"]["provider"] == "piper"
    assert load_config()["tts"]["piper"]["voice"] == "es_ES-davefx-medium"


@pytest.mark.parametrize("provider, voice_config", [
    ("edge", "  edge:\n    voice: es-ES-ElviraNeural\n    speed: 1.2\n"),
    ("piper", "  piper:\n    voice: es_ES-sharvard-medium\n  edge:\n    speed: 1.2\n"),
    ("openai", "  openai:\n    voice: nova\n  edge:\n    speed: 1.2\n"),
])
def test_repeated_setup_overwrites_a_chosen_voice_and_keeps_other_config(hermes_home, monkeypatch, provider, voice_config):
    from hermes_cli.config import get_config_path, load_config

    from hermes_gadget_plugin import setup

    get_config_path().write_text(
        '# Keep this comment\nagent:\n  max_turns: 7\n'
        f'tts:\n  provider: {provider}\n{voice_config}',
        encoding="utf-8",
    )
    monkeypatch.setattr("hermes_cli.setup.prompt", lambda question, default=None, password=False: default)
    for _ in range(2):
        setup.interactive_setup()
        config = load_config()
        assert config["tts"]["provider"] == "piper"
        assert config["tts"]["piper"]["voice"] == "es_ES-davefx-medium"
        assert config["tts"]["edge"]["speed"] == 1.2
        assert config["agent"]["max_turns"] == 7
        assert "# Keep this comment" in get_config_path().read_text(encoding="utf-8")


def test_repeated_setup_preserves_the_initialized_voice(hermes_home, monkeypatch):
    from hermes_cli.config import load_config

    from hermes_gadget_plugin import setup

    monkeypatch.setattr("hermes_cli.setup.prompt", lambda question, default=None, password=False: default)
    setup.interactive_setup()
    expected = load_config()["tts"]
    setup.interactive_setup()
    assert load_config()["tts"] == expected


def test_pair_approves_through_hermes_pairing_store(hermes_home, capsys):
    from gateway.pairing import PairingStore

    from hermes_gadget_plugin import cli

    # As the gateway leaves things: Hermes issued a code for the device, and the plugin noted it.
    code = PairingStore().generate_code("gadget", DEVICE, "Desk")
    devices = cli._store()
    devices.enroll(DEVICE, bytes(32), name="Desk", board="sim-320x240")
    devices.remember_pairing(DEVICE, code, f"hermes pairing approve gadget {code}", 3600)
    assert not PairingStore().is_approved("gadget", DEVICE)

    cli._cmd_pair(types.SimpleNamespace(yes=True, timeout=5))
    assert PairingStore().is_approved("gadget", DEVICE)
    assert "Approved Desk" in capsys.readouterr().out
