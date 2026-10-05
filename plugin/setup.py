"""Hermes Gadget's step in ``hermes gateway setup``.

The wizard runs it when the user picks Hermes Gadget (``register_platform(setup_fn=...)``) and
offers to restart the gateway afterwards. This enables the platform, asks for the port,
applies the branch's voice on every setup, and points at the browser installer and pairing command.
"""

from __future__ import annotations

from . import cli

DEFAULT_PORT = 8765
DEFAULT_TTS_PROVIDER = "piper"
DEFAULT_TTS_VOICE = "es_ES-davefx-medium"
DEFAULT_TTS_NAME = "Davefx (Spanish, Spain)"


def _ui():
    """Hermes's setup prompts, as the built-in platforms use them; plain input and print if they move."""
    try:
        from hermes_cli.setup import print_header, print_info, print_success, print_warning, prompt

        return print_header, print_info, print_success, print_warning, prompt
    except ImportError:
        pass

    def prompt(question: str, default: str | None = None, password: bool = False) -> str:
        answer = input(f"{question}{f' [{default}]' if default else ''}: ").strip()
        return answer or (default or "")

    def say(text: str) -> None:
        print(f"  {text}")

    return (lambda title: print(f"\n  {title}"), say, say, lambda text: say(f"Warning: {text}"), prompt)


def _set_config(key: str, value: str) -> bool:
    """``hermes config set``: Hermes's own config writer, so comments and other settings survive.

    False when nothing was written: no Hermes to import, a package-managed install (whose writer
    only prints an error), or a key or value the writer refuses (it exits)."""
    try:
        from hermes_cli import config
    except ImportError:
        return False
    is_managed = getattr(config, "is_managed", None)
    if is_managed is not None and is_managed():
        return False
    try:
        config.set_config_value(key, value)
    except SystemExit:
        return False
    return True


def interactive_setup() -> None:
    print_header, print_info, print_success, print_warning, prompt = _ui()
    print_header("Hermes Gadget")
    print_info("Small ESP32 devices with a screen, microphone and speaker that talk to this Hermes over your network.")

    extra = cli._gadget_extra()
    current = int(extra.get("port") or DEFAULT_PORT)
    answer = prompt("Port devices connect to", default=str(current))
    try:
        port = int(answer)
        if not 0 < port < 65536:
            raise ValueError
    except ValueError:
        print_warning(f"{answer!r} isn't a port number; keeping {current}")
        port = current

    settings = [("platforms.gadget.enabled", "true")]
    if port != current:
        settings.append(("platforms.gadget.extra.port", str(port)))
    if all(_set_config(key, value) for key, value in settings):
        print_success("Hermes Gadget is enabled")
    else:
        print_warning("Couldn't update config.yaml from here. Run: hermes config set platforms.gadget.enabled true")

    # The voice first: all() stops at a failed write, so the provider never switches without it.
    voice_settings = [(f"tts.{DEFAULT_TTS_PROVIDER}.voice", DEFAULT_TTS_VOICE),
                      ("tts.provider", DEFAULT_TTS_PROVIDER)]
    if all(_set_config(key, value) for key, value in voice_settings):
        print_success(f"Voice configured: {DEFAULT_TTS_NAME} ({DEFAULT_TTS_PROVIDER})")
    else:
        print_warning("Couldn't configure the branch voice. Run these commands on the Hermes host:")
        for key, value in voice_settings:
            print_info(f"  hermes config set {key} {value}")
    print_info("Voice settings apply to this Hermes host, including other chats that use its TTS configuration.")
    if DEFAULT_TTS_PROVIDER == "piper":
        print_info("Piper downloads its voice model on the first spoken reply, then synthesizes offline on the Hermes host.")
    print_info("The voice engine is a declared plugin dependency; enable the plugin through Hermes to install it.")
    print_info("Install ffmpeg on the Hermes host to play speech through the gadget.")

    url = cli.device_url({**extra, "port": port})
    print_info(f"Devices connect to {url}")
    print_info("Once the gateway has restarted, set up a device from Chrome or Edge:")
    print_info(f"  {cli.installer_link(url)}")
    print_info("When the device shows a pairing code, approve it with: hermes gadget pair")
