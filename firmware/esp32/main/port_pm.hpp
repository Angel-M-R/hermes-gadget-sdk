#pragma once
#include "sdkconfig.h"
#include "esp_pm.h"

namespace hgp {
// Guards cover CPU work; queue and audio reads wait without a guard.
class CpuWork {
 public:
  CpuWork() {
#if CONFIG_PM_ENABLE
    static esp_pm_lock_handle_t lock = [] {
      esp_pm_lock_handle_t handle = nullptr;
      esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "hg-work", &handle);
      return handle;
    }();
    lock_ = lock;
    if (lock_) esp_pm_lock_acquire(lock_);
#endif
  }
  ~CpuWork() {
#if CONFIG_PM_ENABLE
    if (lock_) esp_pm_lock_release(lock_);
#endif
  }
  CpuWork(const CpuWork&) = delete;
  CpuWork& operator=(const CpuWork&) = delete;
 private:
#if CONFIG_PM_ENABLE
  esp_pm_lock_handle_t lock_ = nullptr;
#endif
};
}  // namespace hgp
