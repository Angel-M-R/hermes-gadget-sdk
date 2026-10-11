# Talk, type, and interrupt

Wait for the device to show Ready. On the LCD board, BOOT is TALK and PLUS is CANCEL. On the round touchscreen, hold the screen to talk and swipe down to cancel.

| What you want to do | Control |
|---|---|
| Ask a question | Hold TALK, speak, and release to send. A recording stops after one minute |
| Interrupt a spoken reply | Hold TALK to start speaking |
| Discard a recording, close a card, or stop a turn | Tap CANCEL |
| Start a fresh conversation | Hold CANCEL for two seconds |
| Answer a confirmation question | TALK means yes; CANCEL means no |
| Adjust volume or text size | On a touchscreen, tap the title bar or speaker icon to open quick controls |
| Mute or unmute the speaker | Tap **Mute** in quick controls, or hold the speaker icon for 0.4 s. Over USB: `set mute 1` or `set mute 0` |
| Turn the screen off or on | Press PWR on boards that have it (AMOLED-1.8) |
| Type in the simulator | Enter a message in the text box and press Enter |
| Save the simulator's device screen | Press Ctrl+S |
| Pause a finished reply's automatic pages | On a touchscreen, tap **Pause text** in the bottom bar |
| Read the previous or next page | Tap **<** or **>** in the bottom bar; this also pauses automatic paging |
| Resume automatic pages | Tap **Auto pages** |
| Read the last reply again | When the mascot returns, tap **Last reply** in the bottom bar; the text opens paused |

Long replies turn their own pages. Simulated boards with scroll buttons also accept Up and Down.

Touchscreen reading controls appear once the complete reply text arrives. Pausing keeps the current page and prevents the mascot from replacing the reply. It does not pause spoken audio. **Last reply** reads the text already on the device, without asking Hermes again or replaying audio. Starting another message or session replaces that text; restarting the device clears it.

Hold the text area to speak, or tap it in tap-to-talk mode. The bottom reading bar never starts a recording, even if held. Swipe down or press CANCEL to leave a paused reply; if Hermes is still answering, this also stops that turn. The configured screen timeout still applies while reading. The first touch on a sleeping screen only wakes it.

A muted speaker still lets replies play at their own pace, silently, so the text keeps its timing. A screen switched off with PWR stays off while Hermes answers, so you can listen in the dark. A touch, a button, or a question from Hermes turns it back on. On the AMOLED-1.8 the touch controller sleeps while the screen is off, so press BOOT or PWR instead of touching it.

Quick controls change volume in steps of 10 percent and offer **Normal** or **Large** reply text. Spoken audio continues while you adjust them. Tap **Done** or press CANCEL to return to the conversation. A new question from Hermes closes the controls. Volume controls are disabled on boards without a speaker. Holding the title bar for one second opens the full settings menu instead.

In tap-to-talk mode, the device sends your recording after 900 ms of silence by default. Choose a longer pause in settings if it sends before you finish speaking, or tap again to send sooner. Optional recording sounds play quietly at the beginning and end. They are off by default and respect mute and volume. The device waits for the start sound to finish before sending microphone samples.

Select **Español** under **Language** for Spanish device messages and controls. Reply text supports tildes, ñ, ü, ¿ and ¡ in either interface language. This setting does not change the language Hermes uses to answer; ask Hermes for that in the conversation. Unsupported symbols still use the host's text fallback.

Boards with a battery gauge show the charge in the top bar: green while charging, red at 15% or less, and `USB` when no battery is fitted. On boards whose keys sit beside the screen, icons mark them: a microphone next to TALK, lit while recording, and a power symbol next to PWR.

Try asking Hermes to show a reminder on the screen. The agent can use `gadget_display` to display a card. Devices with actions can also respond to requests such as "Turn the LED purple."

If speech is missing, check [audio setup](connect-hermes.md#4-enable-speech). For connection problems, use [troubleshooting](troubleshooting.md). To change Wi-Fi or install firmware updates, see [Manage an existing gadget](setup-board.md#manage-an-existing-gadget).

## Device settings and hardware checks

Hold TALK and CANCEL together for one second to open or close device settings. On a touchscreen, hold the title bar for one second. Settings also work while the device is offline. Opening settings stops the current recording or reply.

Tap CANCEL to move to the next item, then tap TALK to change it. On a touchscreen, swipe down for the next item and tap to select. Boards with Up and Down buttons can move in either direction. Select **Back to Hermes** to leave.

| Item | What it does |
|---|---|
| Speaker volume | Change volume in steps of 10 percent |
| Screen brightness | Choose 10, 25, 50, 75, or 100 percent when the display supports it |
| Talk mode | Choose hold-to-talk or tap-to-talk with silence detection |
| Microphone check | Show the live input level; nothing is saved or sent to Hermes |
| Speaker check | Play a short, quiet tone at the current volume |
| Display check | Show red, green, blue, white, and black bands |
| Input check | Show the last button pressed; release CANCEL to leave the check |
| Device information | Show the board, firmware, device ID, and available audio drivers |
| Battery and power | Show available power readings on boards with a power driver |
| Screen timeout | Choose always on, 30, 60, 120, or 300 seconds |
| Power off | Select twice to shut down a board with a power driver |
| Wi-Fi setup | Start [phone-based setup](setup-board.md#set-up-wi-fi-with-your-phone) on ESP32 boards |
| Pause before sending | Cycle through 900, 1500, or 2400 ms of silence in tap-to-talk mode |
| Recording sounds | Enable or disable local start and end sounds |
| Text size | Choose normal or large reply text |
| Language | Choose English or Español for device messages |

Volume, brightness, talk mode, screen timeout, pause before sending, recording sounds, text size, and language survive restarts. Text starts at normal size and the interface starts in English. Unavailable drivers show as unavailable. These checks help you test the hardware; a completed tone does not prove that a physical speaker produced sound.

The USB console also accepts these settings. `end_silence_ms` accepts any integer from 400 to 3000; hold-to-talk still sends when you release TALK.

```text
set end_silence_ms 1500
set recording_cues 1
set text_size large
set language es
```

Screen timeout is off by default. When enabled, a brightness-capable display dims halfway through the idle period, then goes dark. The first button press or touch wakes it without recording or answering a prompt. Incoming replies and prompts wake it too. Recording, playback, settings, quick controls, pairing and updates keep it awake. The processor and Wi-Fi remain running; this is screen sleep. On the AMOLED-1.8 a dark screen also puts the panel and touch controller to sleep, so a touch does not wake it; press BOOT or PWR. The processor may slow to 80 MHz meanwhile. Use the board's Power off control to shut down the device.

Incoming confirmation prompts and firmware updates close settings. You cannot open settings during a prompt or update. The USB console also accepts `settings`, `settings close`, and `set brightness 50`.
