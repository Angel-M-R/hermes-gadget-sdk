// TCA9554 outputs that power and reset other parts of the board (the
// AMOLED-1.8's panel and touch controller). Register 1 holds the output
// levels, register 3 the directions (1 = input).
#include "port.hpp"  // first: pulls in FreeRTOS.h ahead of task.h/queue.h

#include "esp_log.h"
#include "freertos/task.h"

namespace hgp {
namespace {

const char* TAG = "hg.expander";
constexpr uint8_t kOutput = 0x01;
constexpr uint8_t kConfig = 0x03;

i2c_master_dev_handle_t g_dev = nullptr;
uint8_t g_levels = 0;  // what the output register was last set to

bool write(uint8_t reg, uint8_t value) {
  const uint8_t data[2] = {reg, value};
  return i2c_master_transmit(g_dev, data, sizeof(data), 50) == ESP_OK;
}

}  // namespace

namespace expander {

bool release_resets(const ExpanderResetConfig& cfg, i2c_master_bus_handle_t bus) {
  if (!bus || g_dev) return g_dev != nullptr;
  i2c_device_config_t dev = {};
  dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev.device_address = cfg.addr;
  dev.scl_speed_hz = 400000;
  if (i2c_master_bus_add_device(bus, &dev, &g_dev) != ESP_OK) {
    g_dev = nullptr;
    return false;
  }
  // Levels first, so no pin glitches high when it becomes an output.
  g_levels = cfg.idle;
  if (!write(kOutput, g_levels) || !write(kConfig, static_cast<uint8_t>(~cfg.outputs))) {
    ESP_LOGE(TAG, "TCA9554 at 0x%02x did not answer", cfg.addr);
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(20));
  g_levels |= cfg.outputs;
  if (!write(kOutput, g_levels)) return false;
  vTaskDelay(pdMS_TO_TICKS(150));  // the panel's own start-up after power and reset
  return true;
}

bool pulse(uint8_t bit) {
  if (!g_dev) return false;
  const uint8_t mask = static_cast<uint8_t>(1u << bit);
  if (!write(kOutput, g_levels & ~mask)) return false;
  vTaskDelay(pdMS_TO_TICKS(10));
  if (!write(kOutput, g_levels)) return false;
  vTaskDelay(pdMS_TO_TICKS(50));
  return true;
}

}  // namespace expander
}  // namespace hgp
