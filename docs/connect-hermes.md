# Connect Hermes

Use your own Hermes Agent for replies, tools, memory, and skills. Run the commands below on the computer where Hermes is installed.

## 1. Install the plugin

```bash
HERMES_GADGET_REF=$(git ls-remote https://github.com/Angel-M-R/hermes-gadget-sdk.git refs/heads/face/knight-dragon | cut -f1)
hermes plugins install https://github.com/Angel-M-R/hermes-gadget-sdk.git#plugin --ref "$HERMES_GADGET_REF" --enable
hermes gateway setup
```

Choose **Hermes Gadget**. Restart the gateway when setup asks. Setup prints a device address and an installer link with that address filled in.

Setup on this branch selects **Davefx, Spanish from Spain**, through Piper
(`es_ES-davefx-medium`). Enabling the plugin installs its declared `piper-tts`
dependency through Hermes's package manager. Accept the dependency prompt during
installation; automated installs can add `--yes-deps`.

Every setup run overwrites the current TTS provider and voice with this branch's
values, including existing installations and repeated setup. Voice settings
belong to the Hermes host and also affect other chats using that host's
TTS configuration. Install `ffmpeg` on that computer to decode speech for the
gadget. The voice model downloads on the first spoken reply; subsequent speech
is generated offline on the Hermes host, not on the ESP32. No API key is needed.

**You know it worked when:** `hermes gadget info` shows the gadget configuration and device URL. Keep the gateway running so devices can connect.

The command resolves the latest commit on `face/knight-dragon` and installs that revision. To match a firmware release, use the command in the [release notes](https://github.com/Adolanium/hermes-gadget-sdk/releases), which pins the plugin with `--ref`.

## 2. Connect a device

For hardware, follow [Set up a board](setup-board.md) and use the installer link from `hermes gadget info`.

For a simulator on the same computer as Hermes:

```bash
hermes-gadget sim --url ws://127.0.0.1:8765/gadget --name "Desk Gadget"
```

If Hermes runs on another computer, replace the URL with the address from `hermes gadget info`. That computer's firewall must allow the gadget port, 8765 by default.

## 3. Approve the device

When the device shows a pairing code, run:

```bash
hermes gadget pair
```

Check the device name and code before approving it. You can also run `hermes pairing approve gadget <CODE>` with the code shown on the device.

**You know it worked when:** the device reaches Ready. Type a message in the simulator or use TALK on a board. Replies now come from your Hermes.

## 4. Enable speech

Configure speech recognition and text-to-speech with `hermes tools` and `hermes setup`, or the `stt:` and `tts:` sections of Hermes `config.yaml`. Non-WAV speech output also needs `ffmpeg` on the Hermes computer.

To explicitly select this branch's initial voice for an existing installation:

```bash
hermes config set tts.piper.voice es_ES-davefx-medium
hermes config set tts.provider piper
```

Restart the gateway afterwards. Changing the voice does not require reflashing
the board. Speech recognition remains a separate setup step.

For the desktop simulator, install its [audio extra](desktop.md#4-try-audio) and enable `--live-audio`. Board microphones and speakers use their firmware drivers.

**You know it worked when:** holding TALK captures your speech, releasing it sends the message, and the reply is both displayed and spoken.

Next: [Talk, type, and interrupt](using-gadget.md). For configuration details and plugin development, read [Hermes integration](hermes-integration.md).
