#include "port.hpp"  // first: pulls in FreeRTOS.h ahead of task.h/queue.h

#include <string>

#include "esp_log.h"
#include "esp_pm.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "sdkconfig.h"

namespace hgp {

uint32_t EspSystem::now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

void EspSystem::random_bytes(uint8_t* out, size_t len) {
  // Hardware RNG; entropy is good once the radio is up, and the device key is
  // generated after Wi-Fi starts (see main.cpp).
  esp_fill_random(out, len);
}

void EspSystem::log(hg::LogLevel level, std::string_view message) {
  static const char* TAG = "hg";
  std::string msg(message);
  switch (level) {
    case hg::LogLevel::Debug: ESP_LOGD(TAG, "%s", msg.c_str()); break;
    case hg::LogLevel::Info: ESP_LOGI(TAG, "%s", msg.c_str()); break;
    case hg::LogLevel::Warn: ESP_LOGW(TAG, "%s", msg.c_str()); break;
    case hg::LogLevel::Error: ESP_LOGE(TAG, "%s", msg.c_str()); break;
  }
}

namespace cpu {
namespace {
#if CONFIG_PM_ENABLE
const char* TAG = "hg.cpu";
esp_pm_lock_handle_t g_full_speed = nullptr;
bool g_held = false;
#endif
}  // namespace

void begin() {
#if CONFIG_PM_ENABLE
  // Held before power management starts, so the processor never slows down unasked.
  if (esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "hg-screen", &g_full_speed) != ESP_OK) {
    ESP_LOGW(TAG, "no power management lock: the processor stays at full speed");
    g_full_speed = nullptr;
    return;
  }
  set_full_speed(true);
  // An 80 MHz floor keeps the APB bus, and every peripheral clocked from it, at
  // its usual rate. No light sleep: the codec's I2S runs all the time anyway.
  esp_pm_config_t pm = {};
  pm.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
  pm.min_freq_mhz = 80;
  pm.light_sleep_enable = false;
  const esp_err_t err = esp_pm_configure(&pm);
  if (err != ESP_OK) ESP_LOGW(TAG, "power management off (%s)", esp_err_to_name(err));
#endif
}

void set_full_speed(bool on) {
#if CONFIG_PM_ENABLE
  if (!g_full_speed || on == g_held) return;
  if ((on ? esp_pm_lock_acquire(g_full_speed) : esp_pm_lock_release(g_full_speed)) != ESP_OK) return;
  g_held = on;
  ESP_LOGI(TAG, "processor %s", on ? "at full speed" : "may slow to 80 MHz");
#else
  (void)on;
#endif
}

}  // namespace cpu
}  // namespace hgp
