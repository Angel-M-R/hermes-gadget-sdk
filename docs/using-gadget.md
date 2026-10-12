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

A muted speaker still lets replies play at their own pace, silently, so the text keeps its timing. A screen switched off with PWR stays off while Hermes answers, so you can listen in the dark. A touch, a button, or a question from Hermes turns it back on. On the AMOLED-1.8, touch remains active while the screen is off; the first touch only wakes it. BOOT and PWR also wake it.

Quick controls change volume in steps of 10 percent and offer **Normal** or **Large** reply text. Spoken audio continues while you adjust them. Tap **Done** or press CANCEL to return to the conversation. A new question from Hermes closes the controls. Volume controls are disabled on boards without a speaker. Holding the title bar for one second opens the full settings menu instead.

In tap-to-talk mode, the device sends your recording after 900 ms of silence by default. Choose a longer pause in settings if it sends before you finish speaking, or tap again to send sooner. Optional recording sounds play quietly at the beginning and end. They are off by default and respect mute and volume. The device waits for the start sound to finish before sending microphone samples.

Select **Español** under **Language** for Spanish device messages and controls. Reply text supports tildes, ñ, ü, ¿ and ¡ in either interface language. This setting does not change the language Hermes uses to answer; ask Hermes for that in the conversation. Unsupported symbols still use the host's text fallback.

Enable the battery indicator in settings to show the charge: green while charging, red at 10% or less without USB, and `USB` on rectangular displays when no battery is fitted. On boards whose keys sit beside the screen, icons mark them: a microphone next to TALK, lit while recording, and a power symbol next to PWR.

Try asking Hermes to show a reminder on the screen. The agent can use `gadget_display` to display a card. Devices with actions can also respond to requests such as "Turn the LED purple."

If speech is missing, check [audio setup](connect-hermes.md#4-enable-speech). For connection problems, use [troubleshooting](troubleshooting.md). To change Wi-Fi or install firmware updates, see [Manage an existing gadget](setup-board.md#manage-an-existing-gadget).

## Device settings and hardware checks

Hold TALK and CANCEL together for one second to open or close device settings. On a touchscreen, hold the title bar for one second. Round displays have no title bar; they show a **SETTINGS** target beneath the status dot instead. Hold it for one second. Touches in the target do not start recording. The target is hidden while a question, update or Wi-Fi setup is on screen, because holding it does nothing then. While settings is open, its label reads **BACK TO HERMES**; hold it again for one second to return. Settings also work while the device is offline. Opening settings stops the current recording or reply.

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
| Battery and power | Show available power readings on boards with a power driver; select to choose what the top of the screen shows: icon, percentage, both, or off |
| Screen timeout | Choose always on, 30, 60, 120, or 300 seconds |
| Power off | Select twice to shut down a board with a power driver |
| Pause before sending | Cycle through 900, 1500, or 2400 ms of silence in tap-to-talk mode |
| Recording sounds | Enable or disable local start and end sounds |
| Text size | Choose normal or large reply text |
| Language | Choose English or Español for device messages |
| Wi-Fi setup | Start [phone-based setup](setup-board.md#set-up-wi-fi-with-your-phone) on ESP32 boards, with a QR code for the temporary network where the screen has room |

Volume, brightness, talk mode, battery indicator, screen timeout, mute, pause before sending, recording sounds, text size and language survive restarts. Unavailable drivers show as unavailable. These checks help you test the hardware; a completed tone does not prove that a physical speaker produced sound.

Text starts at normal size, the interface starts in English, and recording sounds are off. The USB console accepts `set end_silence_ms 1500`, `set recording_cues 1`, `set text_size large` and `set language es`. `end_silence_ms` accepts 400..3000; hold-to-talk still sends on release.

The battery indicator is off by default. Each select on **Battery and power** moves to the next choice: icon only, percentage only, both, or off. It sits in the title bar, or centred at the top edge of a round display, and appears only when the board reports a percentage: boards without a fuel gauge or voltage estimate show nothing. The icon fills green while charging and turns red at 10 percent or less without USB; with the percentage alone, the number changes color instead. The percentage is the same estimate as in **Battery and power**. From the USB console, use `set battery_display icon`, `percentage`, `both` or `off`.

Screen timeout is off by default. When enabled, a brightness-capable display dims halfway through the idle period, then goes dark. The first button press or touch wakes it without recording or answering a prompt. Incoming replies and prompts wake it too. Recording, playback, quick controls, Wi-Fi setup, pairing and updates keep it awake. The settings menu sleeps like any other screen, and the input that wakes it leaves the menu on the same item, including a running hardware check. The processor and Wi-Fi remain running; this is screen sleep. AMOLED panels also switch their output off while dark. Use the board's Power off control to shut down the device.

On the AMOLED-1.75C and AMOLED-1.8, a short press of PWR turns the screen off or on. A deliberate screen-off request stays in effect during spoken replies; prompts, updates and connection loss wake it. Wi-Fi setup and prompts cannot be hidden with PWR. PWR, BOOT or touch wakes the screen; the first touch only wakes, without recording. AMOLED output is disabled while dark, but touch polling and the upstream audio clocks keep running.
