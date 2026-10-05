// Board description: which peripherals exist and how they are wired.
//
// Select the board's pins and drivers in board.cpp through the Kconfig choice.
// Board-specific power/reset sequencing runs before peripheral initialization.
// See docs/porting.md for the display, audio, input and power contracts.
#pragma once

#include <cstdint>

namespace hgp {

enum class LcdController { St7789, Box3, CoreS3 };

struct LcdConfig {
  bool enabled = false;
  uint16_t width = 320, height = 240;  // after rotation
  bool swap_xy = true, mirror_x = true, mirror_y = false, invert = true;
  int gap_x = 0, gap_y = 0;
  int mosi = -1, sclk = -1, cs = -1, dc = -1, rst = -1, backlight = -1;
  int spi_mhz = 40;
  LcdController controller = LcdController::St7789;
  bool reset_active_high = false;
};

struct I2sMicConfig {
  bool enabled = false;
  int sck = -1, ws = -1, sd = -1;
};

struct I2sSpeakerConfig {
  bool enabled = false;
  int bclk = -1, ws = -1, dout = -1;
};

// QSPI AMOLED with a CO5300 controller. Each panel has its own start-up
// sequence: the round 466x466 1.75" or the rectangular 368x448 1.8".
enum class AmoledPanel { Round175, Rect18 };

struct AmoledConfig {
  bool enabled = false;
  AmoledPanel panel = AmoledPanel::Round175;
  uint16_t width = 466, height = 466;
  int cs = -1, sclk = -1, d0 = -1, d1 = -1, d2 = -1, d3 = -1, rst = -1;
  int gap_x = 0, gap_y = 0;  // the controller's RAM is wider than the glass
  int qspi_mhz = 40;
  bool round = false;
  int corner_radius = 0;  // rounded glass corners (hg::DisplayInfo::corner_radius)
};

struct I2cBusConfig {
  int sda = -1, scl = -1;
  uint32_t hz = 400000;
};

// ES8311/AW88298 (speaker) and ES7210 (microphone ADC) sharing one duplex I2S bus,
// controlled over the I2C bus. A board with one microphone may take it through the
// ES8311's own ADC instead.
enum class SpeakerCodec { Es8311, Aw88298 };
enum class MicCodec { Es7210, Es8311 };

struct CodecAudioConfig {
  bool enabled = false;
  int mclk = -1, bclk = -1, ws = -1, dout = -1, din = -1;
  int pa = -1;               // speaker amplifier enable, active high
  float amp_supply_v = 5.0f;  // amplifier supply; the ES8311 driver sets its output level from it
  float mic_gain_db = 24.0f;
  SpeakerCodec speaker = SpeakerCodec::Es8311;
  MicCodec mic = MicCodec::Es7210;
};

// Capacitive touch on the I2C bus: hold to talk, tap, swipe down to cancel.
enum class TouchController { Cst9217, Box3, Ft5x06, Cst820 };

struct TouchConfig {
  bool enabled = false;
  uint8_t addr = 0x5A;
  int rst = -1;
  int expander_rst = -1;  // reset on an ExpanderResetConfig output bit instead of a GPIO
  uint16_t width = 0, height = 0;
  bool mirror_x = false, mirror_y = false;
  TouchController controller = TouchController::Cst9217;
};

// A key whose level is read from a TCA9554 I/O expander input (e.g. a PMIC's
// power key). Acts as CANCEL: a press cancels, holding 2 s starts a new session.
struct ExpanderKeyConfig {
  bool enabled = false;
  uint8_t addr = 0x20;
  uint8_t bit = 0;
  bool active_high = true;
};

// TCA9554 outputs that power or reset other parts (the AMOLED-1.8's panel and
// touch controller). At start-up they become outputs at their `idle` levels,
// then all go high together.
struct ExpanderResetConfig {
  bool enabled = false;
  uint8_t addr = 0x20;
  uint8_t outputs = 0;  // bit mask of the pins the firmware drives
  uint8_t idle = 0;     // their levels while the parts are held in reset
};

// A physical key beside the screen, marked by an icon (hg::DeviceProfile::KeyMark):
// the edge it sits on ('l' or 'r'; 0 = no icon) and its offset in pixels from
// the screen's vertical centre.
struct KeyMarkConfig {
  char edge = 0;
  int16_t dy = 0;
};

struct ButtonConfig {
  int talk = -1, cancel = -1, up = -1, down = -1;  // active-low GPIOs, -1 = absent
};

struct LatchPowerConfig {
  bool enabled = false;
  int adc = -1, enable = -1, charging = -1;
};

struct BoardConfig {
  const char* name;
  LcdConfig lcd;
  I2sMicConfig mic;
  I2sSpeakerConfig speaker;
  ButtonConfig buttons;
  AmoledConfig amoled;
  I2cBusConfig i2c;
  CodecAudioConfig codec;
  TouchConfig touch;
  ExpanderKeyConfig pwr_key;
  ExpanderResetConfig expander_resets;
  bool axp2101 = false;
  // PWR reaches the ESP32 only through the AXP2101: a short press turns the screen off or on.
  bool axp_power_key = false;
  KeyMarkConfig talk_key, power_key;
  bool axp_audio_supply = false;
  bool cores3 = false;
  LatchPowerConfig latch_power;
  int status_led = -1;
  const char* talk_label = "TALK";
  const char* cancel_label = "CANCEL";
};

const BoardConfig& board_config();

}  // namespace hgp
