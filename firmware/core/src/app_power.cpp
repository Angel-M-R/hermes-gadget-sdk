#include "hg/app.hpp"

#include <algorithm>

namespace hg {

bool App::wake_display() {
  activity_at_ = now();
  display_off_by_user_ = false;
  const bool sleeping = display_sleeping_;
  if (display_dimmed_ || sleeping) {
    display_dimmed_ = display_sleeping_ = false;
    if (sleeping) hal_.display->set_sleep(false);
    hal_.display->set_backlight(brightness_);
    if (ui_) ui_->invalidate();
    update_model();
  }
  return sleeping;
}

void App::wake_for_activity() {
  if (!display_off_by_user_) wake_display();
}

void App::darken_display() {
  display_sleeping_ = true;
  display_dimmed_ = false;
  hal_.display->set_backlight(0);
  hal_.display->set_sleep(true);
}

void App::on_power_key() {
  if (!hal_.display || !hal_.display->info().has_backlight) return;
  if (display_sleeping_ || display_dimmed_) {
    wake_display();
    return;
  }
  // Phone setup's password, a question and an update's progress stay on screen.
  if (!wifi_setup_text_.empty() || prompt_showing() || ota_busy() || ota_ == Ota::Restarting) return;
  display_off_by_user_ = true;
  darken_display();
}

void App::power_tick() {
  if (hal_.power && (!power_read_at_ || now() - power_read_at_ >= 5000)) {
    power_status_ = hal_.power->read();
    power_read_at_ = now();
    sensors_dirty_ = true;
    update_model();
  }
  constexpr uint32_t kPowerKeyPollMs = 100;
  if (hal_.power && now() - power_key_polled_at_ >= kPowerKeyPollMs) {
    power_key_polled_at_ = now();
    if (hal_.power->take_key_press()) on_power_key();
  }
  if (!hal_.display || !hal_.display->info().has_backlight) return;
  if (display_off_by_user_) {
    // Off stays off through replies; a question or an update still shows.
    if (prompt_showing() || ota_busy() || ota_ == Ota::Restarting || phase_ != Phase::Online) wake_display();
    return;
  }
  if (!screen_timeout_ms_) return;
  const bool idle = mode_ == Mode::Idle && !speaking() && !quick_controls_open_ && !talk_held_ && !cancel_held_ &&
                    !prompt_showing() && wifi_setup_text_.empty() && !ota_busy() && ota_ != Ota::Restarting && overlay_ == Overlay::None &&
                    (phase_ == Phase::NoNetwork || (phase_ == Phase::Online && paired_));
  if (!idle) { wake_display(); return; }
  const uint32_t elapsed = now() - activity_at_;
  if (elapsed >= screen_timeout_ms_ && !display_sleeping_) {
    darken_display();
  } else if (elapsed >= screen_timeout_ms_ / 2 && !display_dimmed_ && !display_sleeping_) {
    display_dimmed_ = true;
    hal_.display->set_backlight(std::min<uint8_t>(brightness_, 10));
  }
}

json::Value App::power_value() const {
  json::Value value = json::Value::object();
  value.set("available", power_status_.has_value());
  if (!power_status_) return value;
  const auto& p = *power_status_;
  if (p.battery_present) value.set("battery_present", *p.battery_present);
  if (p.battery_mv) value.set("battery_mv", *p.battery_mv);
  if (p.battery_percent) value.set("battery_percent", *p.battery_percent);
  if (p.charging) value.set("charging", *p.charging);
  if (p.external_power) value.set("external_power", *p.external_power);
  return value;
}

}  // namespace hg
