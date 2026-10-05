// Touch controllers and a key mirrored on a TCA9554 expander, polled over
// I2C from their own task. Samples become Touch and Key events; the app task
// turns them into gestures (hg::TouchGestures) and button presses.
#include "port.hpp"  // first: pulls in FreeRTOS.h ahead of task.h/queue.h

#include <algorithm>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_touch_tt21100.h"
#include "esp_lcd_touch_ft5x06.h"
#include "freertos/task.h"

namespace hgp {
namespace {

const char* TAG = "hg.touch";
constexpr uint32_t kPollMs = 20;
constexpr uint8_t kTca9554Input = 0x00;
constexpr uint8_t kCstAck = 0xAB;
constexpr uint8_t kCst820Points = 0x02;      // finger count, then X and Y, 12 bits each
constexpr uint8_t kCst820Sleep = 0xE5;        // 0x03: deep sleep until reset
constexpr uint8_t kCst820NoAutoSleep = 0xFE;  // else it stops answering I2C when idle

}  // namespace

bool TouchInput::begin(const TouchConfig& touch, const ExpanderKeyConfig& key, i2c_master_bus_handle_t bus) {
  if (!bus) return false;
  touch_ = touch;
  key_ = key;
  if (touch.enabled && touch.controller == TouchController::Ft5x06) {
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_FT5x06_ADDRESS;
    io_cfg.scl_speed_hz = 100000;
    io_cfg.control_phase_bytes = 1;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.flags.disable_control_phase = 1;
    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_touch_config_t cfg = {};
    cfg.x_max = touch.width;
    cfg.y_max = touch.height;
    cfg.rst_gpio_num = GPIO_NUM_NC;  // The board's expander already released reset.
    cfg.int_gpio_num = GPIO_NUM_NC;
    if (esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io) == ESP_OK &&
        esp_lcd_touch_new_i2c_ft5x06(io, &cfg, &managed_touch_) != ESP_OK) esp_lcd_panel_io_del(io);
  } else if (touch.enabled && touch.controller == TouchController::Box3) {
    begin_box_touch(bus);
  } else if (touch.enabled && touch.controller == TouchController::Cst820) {
    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = touch.addr;
    dev.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &dev, &touch_dev_) == ESP_OK && !reset_cst820()) {
      ESP_LOGW(TAG, "touch controller at 0x%02x did not answer", touch.addr);
    }
  } else if (touch.enabled) {
    if (touch.rst >= 0) {
      gpio_config_t rst = {};
      rst.pin_bit_mask = 1ULL << touch.rst;
      rst.mode = GPIO_MODE_OUTPUT;
      gpio_config(&rst);
      gpio_set_level(static_cast<gpio_num_t>(touch.rst), 0);
      vTaskDelay(pdMS_TO_TICKS(10));
      gpio_set_level(static_cast<gpio_num_t>(touch.rst), 1);
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = touch.addr;
    dev.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &dev, &touch_dev_) == ESP_OK) {
      const uint8_t command_mode[2] = {0xD1, 0x01};
      if (i2c_master_transmit(touch_dev_, command_mode, sizeof(command_mode), 50) != ESP_OK) {
        ESP_LOGW(TAG, "touch controller at 0x%02x did not answer", touch.addr);
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  if (key.enabled) {
    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = key.addr;
    dev.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &dev, &key_dev_) != ESP_OK) key_dev_ = nullptr;
  }
  if (!has_touch() && !key_dev_) return false;
  xTaskCreate(&TouchInput::task, "hg-touch", 3072, this, 5, nullptr);
  ESP_LOGI(TAG, "touch %s, key %s", has_touch() ? "ready" : "off", key_dev_ ? "ready" : "off");
  return true;
}

// Idle since power-up or put to sleep, a CST820 ignores I2C until its reset
// line is pulsed. Afterwards it must be told again not to doze off.
bool TouchInput::reset_cst820() {
  if (touch_.expander_rst >= 0) expander::pulse(static_cast<uint8_t>(touch_.expander_rst));
  const uint8_t stay_awake[2] = {kCst820NoAutoSleep, 0x01};
  return i2c_master_transmit(touch_dev_, stay_awake, sizeof(stay_awake), 50) == ESP_OK;
}

bool TouchInput::can_sleep() const {
  return touch_dev_ && touch_.controller == TouchController::Cst820 && touch_.expander_rst >= 0;
}

bool TouchInput::read_touch(TouchSample& out) {
  if (managed_touch_) {
    if (esp_lcd_touch_read_data(managed_touch_) != ESP_OK) return false;
    uint16_t x = 0, y = 0;
    uint8_t points = 0;
    const bool down = esp_lcd_touch_get_coordinates(managed_touch_, &x, &y, nullptr, &points, 1);
    out = {down && points > 0, static_cast<int16_t>(x), static_cast<int16_t>(y)};
    return true;
  }
  if (touch_.controller == TouchController::Cst820) {
    uint8_t reg = kCst820Points, buf[5] = {};
    if (i2c_master_transmit_receive(touch_dev_, &reg, 1, buf, sizeof(buf), 20) != ESP_OK) return false;
    const bool down = (buf[0] & 0x0F) > 0;
    int x = std::min<int>(((buf[1] & 0x0F) << 8) | buf[2], touch_.width - 1);
    int y = std::min<int>(((buf[3] & 0x0F) << 8) | buf[4], touch_.height - 1);
    if (touch_.mirror_x) x = touch_.width - 1 - x;
    if (touch_.mirror_y) y = touch_.height - 1 - y;
    out = {down, static_cast<int16_t>(x), static_cast<int16_t>(y)};
    return true;
  }
  const uint8_t reg[2] = {0xD0, 0x00};
  if (i2c_master_transmit(touch_dev_, reg, sizeof(reg), 20) != ESP_OK) return false;
  // The controller needs ~2 ms before the read; at least one tick whatever the tick rate.
  vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(2)));
  uint8_t buf[10] = {};
  if (i2c_master_receive(touch_dev_, buf, sizeof(buf), 20) != ESP_OK) return false;
  if (buf[6] != kCstAck) return false;  // not a valid report
  const int points = buf[5] & 0x7F;
  const bool down = points > 0 && (buf[0] & 0x0F) == 0x06;
  int x = (buf[1] << 4) | (buf[3] >> 4);
  int y = (buf[2] << 4) | (buf[3] & 0x0F);
  if (touch_.mirror_x && touch_.width) x = touch_.width - 1 - x;
  if (touch_.mirror_y && touch_.height) y = touch_.height - 1 - y;
  out = {down, static_cast<int16_t>(x), static_cast<int16_t>(y)};
  return true;
}

bool TouchInput::begin_box_touch(i2c_master_bus_handle_t bus) {
  esp_lcd_panel_io_i2c_config_t io_cfg = {};
  io_cfg.scl_speed_hz = 100000;
  io_cfg.control_phase_bytes = 1;
  io_cfg.lcd_cmd_bits = 16;
  io_cfg.flags.disable_control_phase = 1;
  bool tt21100 = false;
  if (i2c_master_probe(bus, 0x5d, 50) == ESP_OK || i2c_master_probe(bus, 0x14, 50) == ESP_OK) {
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
    if (i2c_master_probe(bus, 0x5d, 50) != ESP_OK) io_cfg.dev_addr = 0x14;
  } else if (i2c_master_probe(bus, 0x24, 50) == ESP_OK) {
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_TT21100_ADDRESS;
    tt21100 = true;
  } else {
    return false;
  }
  esp_lcd_panel_io_handle_t io = nullptr;
  if (esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io) != ESP_OK) return false;
  esp_lcd_touch_config_t cfg = {};
  cfg.x_max = touch_.width;
  cfg.y_max = touch_.height;
  cfg.rst_gpio_num = GPIO_NUM_NC;  // Display initialization already reset the shared line.
  cfg.int_gpio_num = GPIO_NUM_3;
  cfg.flags.mirror_x = tt21100;
  const esp_err_t err = tt21100 ? esp_lcd_touch_new_i2c_tt21100(io, &cfg, &managed_touch_)
                              : esp_lcd_touch_new_i2c_gt911(io, &cfg, &managed_touch_);
  if (err != ESP_OK) { esp_lcd_panel_io_del(io); return false; }
  return true;
}

bool TouchInput::read_key(bool& pressed) {
  uint8_t reg = kTca9554Input, value = 0;
  if (i2c_master_transmit_receive(key_dev_, &reg, 1, &value, 1, 20) != ESP_OK) return false;
  bool high = (value >> key_.bit) & 1;
  pressed = key_.active_high ? high : !high;
  return true;
}

void TouchInput::task(void* arg) {
  auto* self = static_cast<TouchInput*>(arg);
  bool was_touching = false, key_down = false;
  bool failed = false;     // the last sleep or wake didn't take
  TickType_t retry_at = 0;  // ...so wait until then; compared only while failed
  for (;;) {
    const bool sleep = self->sleep_wanted_;
    if (sleep == self->asleep_) failed = false;  // nothing left to retry
    if (sleep != self->asleep_ && self->can_sleep() &&
        (!failed || static_cast<int32_t>(xTaskGetTickCount() - retry_at) >= 0)) {
      if (sleep && was_touching) {
        // The finger is lifted for the app before the controller goes quiet.
        TouchSample up{};
        events::post(EventType::Touch, &up, sizeof(up));
        was_touching = false;
      }
      const uint8_t deep_sleep[2] = {kCst820Sleep, 0x03};
      const bool done = sleep ? i2c_master_transmit(self->touch_dev_, deep_sleep, sizeof(deep_sleep), 50) == ESP_OK
                              : self->reset_cst820();
      if (done) {
        self->asleep_ = sleep;
        failed = false;
      } else {
        // Stay as it was and try again: a wake that failed would leave touch dead.
        retry_at = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
        if (!failed) ESP_LOGW(TAG, "touch controller did not %s; retrying", sleep ? "sleep" : "wake");
        failed = true;
      }
    }
    TouchSample s{};
    if (self->has_touch() && !self->asleep_ && self->read_touch(s)) {
      // Every sample while the finger is down (gestures need the motion), plus the lift.
      if (s.touching || was_touching) events::post(EventType::Touch, &s, sizeof(s));
      was_touching = s.touching;
    }
    bool pressed = false;
    if (self->key_dev_ && self->read_key(pressed) && pressed != key_down) {
      key_down = pressed;
      KeySample k{pressed};
      events::post(EventType::Key, &k, sizeof(k));
    }
    vTaskDelay(pdMS_TO_TICKS(kPollMs));
  }
}

}  // namespace hgp
