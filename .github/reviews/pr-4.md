# PR 4 review follow-up

Reviewed all 60 Cubic inline comments and all five Greptile inline comments on `539248f`. Duplicate findings share a correction. One power-policy request is intentionally retained as documented below. This is an engineering review record, not a published user guide.

| Item | Comment | Disposition and evidence |
|---|---|---|
| 1 | [plugin/store.py:82](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584200) | Fixed: a shared OS file lock covers the complete registry transaction. Concurrent-instance regression test prevents a forgotten key from returning. |
| 2 | [linux/install.sh:65](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584204) | Fixed: WebSocket client added to the hash-locked runtime inputs; fresh Linux installs verify its import. |
| 3 | [.github/workflows/release.yml:39](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584206) | Fixed: release gate requires successful runs from main for the tagged commit. |
| 4 | [firmware/core/third_party/qrcode/qrcode.cpp:812](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584217) | Fixed: locked QR tables reject another version before touching the supplied buffer. A separate LOCK_VERSION=3 executable tests this mode. |
| 5 | [linux/install.sh:114](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584222) | Fixed: nonzero service start takes the rollback path. Disposable Linux container exercises this failure. |
| 6 | [linux/install.sh:120](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584224) | Fixed: rollback clears systemd start-limit state before restarting the previous release. |
| 7 | [firmware/sim/src/hgsim.cpp:91](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584226) | Fixed: storage callbacks return success through ABI 7. Native-core tests cover an explicit false result and a host exception. |
| 8 | [firmware/esp32/main/port_audio.cpp:181](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584229) | Fixed: I2S writes resume after partial progress, with bounded no-progress retries and flush cancellation. Host tests verify exact output bytes. |
| 9 | [firmware/esp32/main/port_audio.cpp:80](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584233) | Fixed: microphone reads keep samples returned with a timeout; permanent errors remain rejected. |
| 10 | [firmware/esp32/main/main.cpp:209](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584239) | Fixed: AMOLED startup is gated by successful peripheral reset/power initialization. |
| 11 | [firmware/core/src/qr.cpp:52](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584241) | Fixed: wrapper and raw encoder reject invalid correction levels before shifts/table access. |
| 12 | [firmware/drivers/ws_link.hpp:20](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584245) | Fixed: an old-generation loss cannot replace a current-generation marker. Host regression covers reversed callback order. |
| 13 | [firmware/drivers/wifi_retry.hpp:61](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584250) | Fixed: start-failure scheduling avoids zero across clock wrap. Host regression covers this boundary. |
| 14 | [firmware/core/src/qr.cpp:54](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584256) | Fixed: QR wrapper refuses lengths above UINT16_MAX before narrowing. |
| 15 | [firmware/esp32/main/panel_ws185.hpp:378](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584261) | Fixed: parameterless panel commands have zero-length payloads in the newer vendor table. |
| 16 | [firmware/esp32/main/port_diag.cpp:134](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584264) | Fixed: I2C scan uses a two-second budget and reports an incomplete scan. Fault simulation bounds nonconsecutive timeouts. |
| 17 | [firmware/drivers/wifi_retry.hpp:44](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584267) | Fixed: silent attempt timeouts use the same battery backoff as disconnects. Repeated timeout regression checks increasing waits. |
| 18 | [firmware/core/src/ui.cpp:319](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584270) | Fixed: render hash includes every QR module. Same-size replacement regression requires different framebuffer output. |
| 19 | [firmware/esp32/main/main.cpp:180](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584276) | Fixed: CPU-frequency guards cover app dispatch and audio sample processing on power-managed profiles. Physical frequency measurements remain pending. |
| 20 | [firmware/esp32/main/port_wifi.cpp:49](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584280) | Fixed: display wake and detected USB reconnection request an immediate Wi-Fi retry. Core wake callback has a regression test. |
| 21 | [firmware/tests/test_app.cpp:2481](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584284) | Fixed: renderer test compares every module and quiet-zone pixel with the encoded matrix. |
| 22 | [firmware/esp32/main/Kconfig.projbuild:45](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584287) | Fixed: board selector explicitly identifies AMOLED-1.8 V2 only. |
| 23 | [docs/setup-board.md:54](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584293) | Fixed: guide scopes board-tag protection to OTA and requires manual model/revision verification for direct USB flashing. |
| 24 | [docs/hermes-integration.md:129](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584300) | Fixed: live receive traffic refreshes persisted activity at most once a minute. Virtual-clock session remains pending and selectable past one hour. |
| 25 | [firmware/drivers/mic_power.hpp:58](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584309) | Fixed: codec delivery rechecks the live capture flag after its blocking read, following the I2S microphone pattern. |
| 26 | [linux/install.sh:34](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584314) | Fixed: health timeout accepts only positive decimal values in 1..9999; container checks a nonnumeric override exits before switching. |
| 27 | [firmware/core/src/ui.cpp:460](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584319) | Fixed: round top strip draws USB without a battery and hashes power changes. Appearance/removal framebuffer regression passes. |
| 28 | [plugin/hub.py:568](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584323) | Fixed: first-time authorization is checked before pending caps. Approved enrollment test passes with a full queue. |
| 29 | [plugin/store.py:66](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584326) | Fixed: pruning an expired device also deletes its pairing record and persists the cleanup. Disk-content regression checks both maps. |
| 30 | [THIRD_PARTY_NOTICES.md:39](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584335) | Fixed: schematic attribution explicitly names AMOLED-1.75C. |
| 31 | [firmware/esp32/boards/esp32s3-touch-amoled-18/sdkconfig.defaults:17](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584340) | Kept upstream policy: the migration deliberately retains the upstream fixed-frequency AMOLED-1.8 configuration. Dynamic scaling changes the audio/power baseline and needs a separate physical report. It is documented in development.md; no idle power saving is claimed for this profile. |
| 32 | [linux/hermes-gadget-device:10](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584344) | Fixed: manual rollback shares the install lock. Container holds that lock and verifies rollback refuses to overlap. |
| 33 | [tests/test_hub_pacing.py:89](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584349) | Fixed: virtual-clock pacing test asserts both the earliest and latest allowed send time. |
| 34 | [firmware/esp32/boards/esp32s3-touch-amoled-175c/sdkconfig.defaults:15](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584354) | Fixed: CPU-frequency guards cover processing on the power-managed AMOLED-1.75C profile; waits release the guard. |
| 35 | [firmware/core/src/canvas.cpp:159](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584356) | Fixed: wrapping counts each long word once and decrements the remaining character count after splitting. |
| 36 | [firmware/esp32/main/port_codec.cpp:120](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584361) | Fixed: a separate ES7210 ADC is created only when that microphone is configured; an unsupported analog-mic combination no longer advertises an ES7210. |
| 37 | [plugin/compat.py:76](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584369) | Fixed: constructor checks the required per-instance TTS field after base initialization. Compatibility tests cover its absence. |
| 38 | [docs/porting.md:52](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584374) | Fixed: porting guide describes the RGB scanout configuration and driver. |
| 39 | [plugin/cli.py:231](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584378) | Fixed: pair prefers an exact ID and rejects an ambiguous name before asking for approval. Test keeps both devices unapproved. |
| 40 | [firmware/esp32/main/port_touch.cpp:231](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584383) | Fixed: ISR queues encoder transitions independently of I2C polling; host decoder tests preserve several detents per poll. Physical direction/timing are still unverified. |
| 41 | [firmware/esp32/main/port_codec.cpp:166](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584386) | Fixed: failed microphone task creation frees its buffer, closes a switchable ADC and reports no microphone capability. |
| 42 | [plugin/compat.py:38](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584392) | Fixed: optional CLI/setup imports no longer block adapter startup; their existing fallbacks remain. |
| 43 | [firmware/tests/test_wifi_retry.cpp:15](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584395) | Fixed: retry test bounds its polling loop and fails an assertion instead of hanging. |
| 44 | [docs/using-gadget.md:75](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584403) | Fixed: both daily-use sections say touch remains active while the AMOLED-1.8 display is off. |
| 45 | [firmware/esp32/main/port_parallel.cpp:118](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584407) | Fixed: failure guard frees the I80 panel, IO, bus, semaphore and buffers when initialization fails. |
| 46 | [tests/test_versions.py:49](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584411) | Fixed: restored the missing-version regression alongside changelog checks. |
| 47 | [tests/test_hermes_compat.py:82](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584414) | Fixed: inherited-hook guard includes super calls even when the adapter overrides the same name. |
| 48 | [firmware/esp32/main/port_codec.cpp:164](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584418) | Fixed: failed mic-buffer allocation closes a switchable ADC before returning unavailable. |
| 49 | [SECURITY.md:24](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584423) | Fixed by activity refresh: the one-hour contact-based description is now implemented, including pong traffic. Security wording remains accurate. |
| 50 | [firmware/esp32/main/port_display.cpp:34](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584428) | Fixed: SPI initialization failure guard also handles unknown ST77916 ID after allocation and releases the owned bus/resources. |
| 51 | [firmware/drivers/tag_scanner.hpp:27](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584431) | Fixed: tag matching preserves overlapping prefixes. Chunked aaab/aab regression finds the correct value. |
| 52 | [firmware/core/third_party/qrcode/qrcode.cpp:881](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584433) | Fixed: raw text encoder checks strlen before narrowing. Locked-mode test refuses 65,536 bytes. |
| 53 | [firmware/esp32/main/port_storage.cpp:21](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584438) | Fixed: NVS keys containing NUL are rejected before conversion to a C string. |
| 54 | [.github/workflows/ci.yml:36](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584444) | Fixed: CI declares pull-requests read permission for its label lookup. |
| 55 | [tests/test_firmware_source.py:15](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584446) | Fixed: source guard masks comments and literals with string-aware matching. URL-before-call regression keeps the actual call visible. |
| 56 | [firmware/esp32/main/port_amoled.cpp:114](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584449) | Fixed: AMOLED initialization failure guard releases IO, owned SPI bus, semaphore and buffers. |
| 57 | [NOTICE:57](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584452) | Fixed: AMOLED-1.8 pin map and TCA9554 reset facts retain attribution to the pinned Waveshare BSP. |
| 58 | [plugin/adapter.py:337](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584463) | Fixed: per-device file locks and staged-image IDs serialize staging and completion. Queue and real-adapter tests keep replacement images pending. |
| 59 | [firmware/tests/test_qr.cpp:37](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584473) | Fixed: test returns after reporting failed encoding before inspecting modules. |
| 60 | [CONTRIBUTING.md:50](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4242584477) | Fixed: contribution guide accurately says CI checks that CHANGELOG.md changed. |
| 61 | [linux/install.sh:65](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4246619030) | Same fix as item 2; verified by fresh hash-locked installation and import in Linux. |
| 62 | [linux/install.sh:114](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4246619047) | Same fix as item 5; nonzero service start is exercised in the Linux rollback suite. |
| 63 | [plugin/store.py:125](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4246619054) | Same fix as item 1; registry read/modify/write shares a cross-process lock. |
| 64 | [plugin/store.py:65](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4246619067) | Same fix as items 24 and 49; connected pending activity refresh is rate-limited and regression-tested. |
| 65 | [firmware/esp32/main/port_ws.cpp:75](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4246619074) | Fixed: WebSocket close and callbacks share a receive mutex; generation is checked under the mutex before all buffer/opcode access. Old callbacks finish or are rejected before reuse. SDK builds validate integration; concurrent callbacks were not fault-injected on a physical ESP32. |

## Verification limits

Host tests exercise file races, pending-device activity, OTA replacement, QR matrices, transfer progress, retry scheduling and input decoding. GPIO ISR timing, codec startup failures, display-driver allocation failures and WebSocket callback concurrency also received source review and ESP-IDF compilation checks; no physical ESP32 fault injection or audio measurements are claimed. Hardware remains experimental.

Validation results are recorded in the PR description.


## Local validation on the corrected source

- `cmake --build build/host --parallel 4` and `ctest --test-dir build/host --output-on-failure`: 190 core cases plus the locked-version QR executable passed.
- `python -m pytest -q -rs --ignore=tests/test_sim_window.py`: 280 passed, 38 skipped on macOS. Hermes-only checks and the Linux process/socket check ran separately below.
- `HERMES_AGENT_DIR=build/review-hermes-agent HERMES_GADGET_E2E=1 build/review-hermes-agent/.venv/bin/python -m pytest -q tests/test_adapter_hermes.py tests/test_gateway_e2e.py tests/test_plugin_install_hermes.py tests/test_setup_hermes.py tests/test_hermes_compat.py`: 43 passed against pinned Hermes `2eceb0275501b09febdec88d08b80d1fff730cf8`. A real Hermes gateway runs with test doubles for the model, STT and TTS services.
- `sh linux/test_install.sh <fresh-linux-package>` in a disposable Debian Trixie ARM64 container: passed startup, reinstall, failed dependency install, health timeout, nonzero start, rollback exclusion, state preservation and pruning. The native service process runs for real; systemctl is a stand-in because the container has no init system. The test asserts the reset-failed command; it does not simulate systemd rate limiting itself.
- `HGSIM_LIBRARY=/tmp/host/libhgsim.so HERMES_GADGET_UI_TESTS=1 xvfb-run -a python -m pytest -q tests/test_sim_window.py tests/test_linux.py` in that container: 11 passed, including six UI cases and five Linux client cases.
- `shellcheck linux/install.sh linux/hermes-gadget-device linux/test_install.sh`: passed in the container.
- `npm test` and `npm run build` in site: 29 tests passed and 19 documentation pages built.
- `pio run -j 4` in firmware/esp32: all 12 profiles passed with PlatformIO 6.1.19 and ESP-IDF 6.1.0.
- `python firmware/esp32/tools/package_release.py --all --out firmware/esp32/dist --expect-version 0.2.0`: all profiles packaged, 25 SHA256SUMS entries verified, notice and license files checked inside the archive.
- `python tools/check_versions.py`: 0.2.0 in all four files. `git diff --check`: passed.

Before the fixes, the new registry/approval/activity tests failed and the new QR length, overlapping-tag and retry-wrap tests failed. An early gateway run exposed test instrumentation still observing the old terminal-report method; it now observes the atomic write while preserving its assertions and tests replacement at the real adapter call site. The final gateway suite passes. macOS UI attempts could not initialize Tcl: the managed Python lacked a usable init.tcl and Homebrew Python lacked _tkinter. UI validation therefore ran on Debian with Tk and Xvfb. Initial container setup attempts lacked make and Git; the final environment includes make and calls the package builder directly with an explicit revision.

No USB flashing, physical audio measurement, frequency/power measurement or experimental-port verification was performed. Remote CI must validate the pushed revision before any merge.

## Python 3.10 CI follow-up

The concurrency regression correctly waited on the file lock, but expected the built-in TimeoutError. Python 3.10's Future.result raises concurrent.futures.TimeoutError, which is a separate class there. Importing that exception explicitly preserves the same blocking assertion on every supported Python version. The isolated regression failed before this two-line correction and the plugin unit suite passed afterward on Python 3.10 and 3.13. Production code is unchanged.


## Review of the follow-up commit

- [tests/test_plugin_units.py:148](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247213718): Addressed in ddbb890: the Python 3.10 concurrency assertion imports the futures exception. All 18 plugin unit tests passed on Python 3.10 and 3.13.
- [linux/test_install.sh:134](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247213722): Addressed: the nonzero-start test searches only log entries added by that install attempt, so a prior rollback cannot satisfy the reset-failed assertion.
- [THIRD_PARTY_NOTICES.md:31](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247213731): Addressed: distinguish internal write bounds during encoding, payload capacity rejection after codeword encoding, pre-encoding input/ECC/version checks, and the caller-owned module buffer contract.
- [docs/linux.md:331](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247213752): Addressed: startup and automatic health-check rollback apply only when the service was running before the update; a previously stopped service stays stopped.

Follow-up validation: 125 documentation/version/package/plugin tests passed on Python 3.10; the strengthened Linux installer suite and shellcheck passed in a disposable Debian ARM64 container, followed by 11 UI/Linux client cases. The site built all 19 guides. Distribution notices and checksums were regenerated and checked against the source. No production code changed in this follow-up.

## Review of ddbb890

All eight additional inline comments were evaluated, including the OTA finding that the summary no longer listed.

- [docs/development.md](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246613): synchronize both base dependencies and extras with the Pi lock input.
- [firmware/tests/test_quadrature.cpp](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246643): a diagonal transition now follows a nonzero partial count. The next three legal transitions must not emit a detent. A copied-header mutation that removes the reset fails this probe; the corrected decoder passes.
- [firmware/esp32/main/port_pm.hpp](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246653): lock creation, acquisition, and release errors are fatal. The abort fallback also applies when NDEBUG disables ESP_ERROR_CHECK. A host probe with ESP-IDF API doubles checked balanced nested guards and each injected error with assertions enabled and disabled. These are simulations, not physical frequency measurements.
- [plugin/ota.py](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246660): status, attempt counting, and completion validate the stage ID from metadata under the existing lock. They no longer allocate the image. Despite the bot's addressed marker, the prior source still reread the image; an instrumented comparison reproduced that read before this correction. A regression refuses image reads during all three operations.
- [plugin/filelock.py](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246666): retry nonblocking Windows lock conflicts without a fixed attempt limit; propagate unrelated errors. Test doubles exercise twelve conflicts before success and an invalid-handle failure. Native Windows contention has not been measured locally. Python documents the ten-attempt limit of LK_LOCK: [msvcrt](https://docs.python.org/3.10/library/msvcrt.html).
- [docs/hermes-integration.md](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246677): persisted device activity and pairing-code expiry are distinct. The guide says to reconnect for a new code after expiry.
- [firmware/esp32/main/port_touch.cpp](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246695): retain a failed detent and retry it before consuming another sample. A host regression fills the destination queue, retries while still full, then confirms all forward and reverse detents arrive exactly once in order. ISR queue overflow remains bounded and logged; fast turns still need physical validation.
- [docs/simulator.md](https://github.com/Angel-M-R/hermes-gadget-sdk/pull/4#discussion_r4247246705): distinguish logged identity/volume/brightness/mute failures from serial-console errors for set commands.

Validation for this round: 192 C++ core cases plus the locked-version QR executable passed; 284 Python tests passed with 38 environment-dependent skips; 142 relevant Python 3.10 tests passed; all 43 pinned-Hermes integration cases passed with model/STT/TTS doubles. Site tests passed (29), all 19 guide pages built, and all 12 firmware profiles compiled with ESP-IDF 6.1.0 after the CPU-lock correction. No changes to vendor material or additional licensing requirements apply. The Windows lock and CPU error probes use test doubles; no board was flashed.
