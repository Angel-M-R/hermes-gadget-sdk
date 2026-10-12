"""The firmware's source: patterns that broke the device and must not come back."""

from __future__ import annotations

import re

from conftest import REPO

FIRMWARE = REPO / "firmware"


def _code_lines(path):
    """Code only, preserving line numbers and excluding literals and comments."""
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/', re.DOTALL)
    source = tokens.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), path.read_text(encoding="utf-8"))
    yield from enumerate(source.splitlines(), start=1)


def test_a_url_before_a_call_does_not_hide_code(tmp_path):
    path = tmp_path / "code.cpp"
    path.write_text('const char* u = "https://x"; esp_websocket_client_close(c); // comment\n'
                    '/* esp_websocket_client_close(c); */\n'
                    'const char* s = "esp_websocket_client_close(c)";\n')
    lines = list(_code_lines(path))
    assert "esp_websocket_client_close(c)" in lines[0][1]
    assert "esp_websocket_client_close" not in lines[1][1] + lines[2][1]


def test_firmware_never_calls_the_websocket_close_that_ignores_its_timeout():
    # esp_websocket_client_close() sends its close frame with no time limit (esp_websocket_client
    # 1.8.0), whatever timeout it is given. With Wi-Fi gone that write never finishes, and the app task
    # that called it stopped drawing, reading touch and retrying Wi-Fi until a hard reset. Closing goes
    # through hg::ws::process() (drivers/ws_link.hpp), which limits every wait.
    call = re.compile(r"\besp_websocket_client_close(_with_[a-z_]+)?\s*\(")
    found = []
    for path in sorted(FIRMWARE.rglob("*")):
        if path.suffix not in (".c", ".cpp", ".h", ".hpp") or "managed_components" in path.parts \
                or ".pio" in path.parts:
            continue
        for number, code in _code_lines(path):
            if call.search(code):
                found.append(f"{path.relative_to(REPO)}:{number}: {code.strip()}")
    assert not found, "\n".join(found)
