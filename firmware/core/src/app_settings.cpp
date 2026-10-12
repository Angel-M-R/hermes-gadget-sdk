#include "hg/app.hpp"

#include <algorithm>
#include <vector>

namespace hg {

bool App::settings_hold_live() const { return !prompt_showing() && !ota_busy() && !wifi_setup_open(); }

bool App::settings_title_hit(int x, int y) const {
  return ui_ && settings_hold_live() && ui_->title_hit(x, y);
}

bool App::open_settings() {
  if (prompt_showing() || ota_busy() || ota_ == Ota::Restarting || !wifi_setup_text_.empty()) return false;
  wake_display();
  if (settings_open()) { close_settings(); return true; }
  quick_controls_open_ = false;
  if (mode_ == Mode::Listening) cancel_listening("local settings");
  else if (mode_ == Mode::Thinking || mode_ == Mode::Responding) cancel_turn();
  stop_playback();
  dismiss_overlay();
  menu_ = Menu::Volume;
  power_off_armed_ = false;
  check_result_.clear();
  update_model();
  return true;
}

void App::stop_hardware_check() {
  if (hardware_check_ == HardwareCheck::Microphone && hal_.mic) hal_.mic->stop();
  if (hardware_check_ == HardwareCheck::Speaker && hal_.speaker) {
    hal_.speaker->abort();
    apply_volume();
  }
  hardware_check_ = HardwareCheck::None;
  level_ = 0;
}

void App::close_settings() {
  stop_hardware_check();
  menu_ = Menu::Closed;
  check_result_.clear();
  if (ui_) ui_->invalidate();
  update_model();
}

void App::settings_input(Button button, bool pressed) {
  if (hardware_check_ == HardwareCheck::Inputs) {
    if (pressed) {
      const char* labels[] = {"TALK", "CANCEL", "UP", "DOWN"};
      check_result_ = std::string(labels[static_cast<unsigned>(button)]) + " pressed";
      return;
    }
    if (button != Button::Cancel) return;
  }
  if ((button == Button::Cancel && !pressed) ||
      ((button == Button::Up || button == Button::Down) && pressed)) {
    stop_hardware_check();
    check_result_.clear();
    power_off_armed_ = false;
    int item = static_cast<int>(menu_) + (button == Button::Up ? -1 : 1);
    if (item < static_cast<int>(Menu::Volume)) item = static_cast<int>(Menu::Back);
    if (item > static_cast<int>(Menu::Back)) item = static_cast<int>(Menu::Volume);
    menu_ = static_cast<Menu>(item);
    const bool can_power_off = hal_.power && hal_.power->can_power_off();
    if ((!hal_.power && menu_ == Menu::Power) ||
        (!can_power_off && menu_ == Menu::PowerOff)) {
      menu_ = menu_ == Menu::Power ? (button == Button::Up ? Menu::Info : Menu::IdleTimer)
                                   : (button == Button::Up ? Menu::IdleTimer : Menu::WifiSetup);
    }
    if (!on_wifi_setup && menu_ == Menu::WifiSetup)
      menu_ = button == Button::Up ? (can_power_off ? Menu::PowerOff : Menu::IdleTimer) : Menu::EndSilence;
    return;
  }
  if (button != Button::Talk || pressed) return;
  if (hardware_check_ != HardwareCheck::None) {
    stop_hardware_check();
    check_result_ = tr("Check stopped");
    return;
  }
  check_result_.clear();
  switch (menu_) {
    case Menu::Volume:
      if (hal_.speaker) console("set volume " + std::to_string(volume_ >= 100 ? 0 : std::min(100, volume_ + 10)));
      break;
    case Menu::Brightness:
      if (hal_.display && hal_.display->info().has_backlight) {
        const int next = brightness_ < 25 ? 25 : brightness_ < 50 ? 50 : brightness_ < 75 ? 75 : brightness_ < 100 ? 100 : 10;
        console("set brightness " + std::to_string(next));
      }
      break;
    case Menu::TalkMode:
      if (hal_.mic) console(talk_mode_ == TalkMode::Hold ? "set talk_mode tap" : "set talk_mode hold");
      break;
    case Menu::EndSilence:
      console("set end_silence_ms " + std::to_string(end_silence_ms_ < 1500 ? 1500 : end_silence_ms_ < 2400 ? 2400 : 900));
      break;
    case Menu::RecordingCues: console(recording_cues_ ? "set recording_cues 0" : "set recording_cues 1"); break;
    case Menu::TextSize: console(large_text_ ? "set text_size normal" : "set text_size large"); break;
    case Menu::Language: console(language_ == Language::Spanish ? "set language en" : "set language es"); break;
    case Menu::Microphone:
      if (hal_.mic && hal_.mic->start(profile_.mic_rate)) hardware_check_ = HardwareCheck::Microphone;
      else check_result_ = tr("Microphone unavailable");
      break;
    case Menu::Speaker:
      if (hal_.speaker && hal_.speaker->begin(profile_.speaker_rate)) {
        hal_.speaker->set_volume(volume_);  // a check is heard even while muted; leaving it restores mute
        // A quiet quarter-second square tone, entirely local to the device.
        std::vector<int16_t> tone(profile_.speaker_rate / 4);
        const uint32_t half_period = std::max<uint32_t>(1, profile_.speaker_rate / 1000);
        for (size_t i = 0; i < tone.size(); ++i) tone[i] = (i / half_period) % 2 ? 900 : -900;
        hal_.speaker->write(tone.data(), tone.size());
        hal_.speaker->end();
        hardware_check_ = HardwareCheck::Speaker;
        check_result_ = tr("Listen for a short tone");
      } else check_result_ = tr("Speaker unavailable");
      break;
    case Menu::Display:
      if (hal_.display) hardware_check_ = HardwareCheck::Display;
      else check_result_ = tr("Display unavailable");
      break;
    case Menu::Inputs:
      hardware_check_ = HardwareCheck::Inputs;
      check_result_ = tr("Press a button. Release Cancel to leave.");
      break;
    case Menu::IdleTimer: {
      const uint32_t seconds = screen_timeout_ms_ / 1000;
      const uint32_t next = seconds == 0 ? 30 : seconds < 60 ? 60 : seconds < 120 ? 120 : seconds < 300 ? 300 : 0;
      console("set screen_timeout " + std::to_string(next));
      break;
    }
    case Menu::PowerOff:
      if (!power_off_armed_) {
        power_off_armed_ = true;
        check_result_ = tr("Select again to power off. Cancel goes back.");
      } else {
        power_off_armed_ = false;
        check_result_ = hal_.power->power_off() ? tr("Power-off requested") : tr("Power-off failed. Try the physical PWR key.");
      }
      break;
    case Menu::Back: close_settings(); break;
    case Menu::WifiSetup: start_wifi_setup(); break;
    case Menu::Power:
      if (battery_readable()) {
        static const char* const kNext[] = {"percentage", "both", "off", "icon"};  // after Icon, Percentage, Both, Off
        console(std::string("set battery_display ") + kNext[static_cast<int>(battery_display_)]);
      }
      break;
    case Menu::Info:
    case Menu::Closed: break;
  }
}

void App::settings_tick() {
  if (talk_held_ && cancel_held_ && !settings_chord_fired_ &&
      now() - talk_down_at_ >= 1000 && now() - cancel_down_at_ >= 1000) {
    if (open_settings()) {
      settings_chord_fired_ = true;
      cancel_long_fired_ = true;
    }
  }
  if (hardware_check_ == HardwareCheck::Speaker && !hal_.speaker->busy()) {
    hardware_check_ = HardwareCheck::None;
    apply_volume();  // the tone played at the saved volume even if muted
    check_result_ = tr("Tone finished. Did you hear it?");
    update_model();
  }
}

bool App::battery_readable() const {
  return power_status_ && power_status_->battery_percent && power_status_->battery_present != false;
}

void App::settings_model() {
  UiModel& m = model_;
  m.screen = Screen::Settings;
  m.headline = tr("Settings");
  m.scroll = 0;
  m.speaking = false;
  m.hint = profile_.touch_screen ? tr("Tap: change | Swipe: next")
                                : profile_.talk_label + tr(": change | ") + profile_.cancel_label + tr(": next");
  switch (menu_) {
    case Menu::Volume:
      m.detail = tr("Speaker volume");
      m.body = !hal_.speaker ? tr("No speaker driver is active.")
               : muted_       ? std::to_string(volume_) + tr("%, muted\n") +
                                    (profile_.touch_screen ? tr("Leave settings, then hold the speaker icon to unmute.") : "Console: set mute 0")
                              : std::to_string(volume_) + tr("%\nChanges are saved.");
      break;
    case Menu::Brightness:
      m.detail = tr("Screen brightness");
      m.body = hal_.display && hal_.display->info().has_backlight ? std::to_string(brightness_) + tr("%\nChanges are saved.")
                                                               : tr("Brightness control is unavailable.");
      break;
    case Menu::TalkMode:
      m.detail = tr("Talk mode");
      m.body = !hal_.mic ? tr("No microphone driver is active.") : talk_mode_ == TalkMode::Hold ? tr("Hold to record; release to send.")
                                                                                         : tr("Tap to record; pause or tap to send.");
      break;
    case Menu::EndSilence:
      m.detail = tr("Pause before sending");
      m.body = std::to_string(end_silence_ms_) + " ms\n" + tr("In tap mode, send after this much silence. Tap again to send sooner.");
      break;
    case Menu::RecordingCues:
      m.detail = tr("Recording sounds");
      m.body = std::string(recording_cues_ ? tr("On") : tr("Off")) + "\n" + tr("Quiet start/end tones. Muting the speaker also mutes these sounds.");
      break;
    case Menu::TextSize:
      m.detail = tr("Text size");
      m.body = large_text_ ? tr("Large") : tr("Normal");
      break;
    case Menu::Language:
      m.detail = tr("Language");
      m.body = language_ == Language::Spanish ? "Español" : "English";
      break;
    case Menu::Microphone:
      m.detail = tr("Microphone check");
      m.body = hardware_check_ == HardwareCheck::Microphone ? tr("Speak now. Level: ") + std::to_string(level_) + tr("%\nLocal only. Nothing is sent.")
                                                          : tr("Start to check the microphone level. No recording is saved or sent.");
      break;
    case Menu::Speaker:
      m.detail = tr("Speaker check");
      m.body = tr("Start to play a short tone at the current volume.");
      break;
    case Menu::Display:
      m.detail = tr("Display check");
      m.body = tr("Start to show red, green, blue, white and black bands.");
      m.color_test = hardware_check_ == HardwareCheck::Display;
      break;
    case Menu::Inputs:
      m.detail = tr("Input check");
      m.body = tr("Start to check the controls. Cancel leaves this check.");
      break;
    case Menu::Info:
      m.detail = tr("Device information");
      m.body = profile_.board + "\nFirmware " + profile_.firmware + "\n" + device_id_ + tr("\nMicrophone: ") +
               (hal_.mic ? tr("available") : tr("unavailable")) + tr("\nSpeaker: ") + (hal_.speaker ? tr("available") : tr("unavailable"));
      break;
    case Menu::Power:
      m.detail = tr("Battery and power");
      if (!power_status_) m.body = tr("Power readings unavailable.");
      else {
        const auto& p = *power_status_;
        if (p.battery_present && !*p.battery_present) m.body = tr("No battery detected.\n");
        if (p.battery_percent) m.body += std::to_string(*p.battery_percent) + tr("% (gauge estimate)\n");
        if (p.battery_mv) m.body += std::to_string(*p.battery_mv) + " mV\n";
        if (p.charging) m.body += *p.charging ? tr("Charging\n") : tr("Not charging\n");
        if (p.external_power) m.body += *p.external_power ? tr("USB power\n") : tr("No USB power\n");
        if (p.battery_percent && *p.battery_percent <= 10 && p.external_power == false) m.body += std::string(tr("Low battery: connect USB.")) + "\n";
      }
    if (battery_readable()) {  // nothing to choose when no percentage can appear
      static const char* const kShown[] = {"Icon", "Percent", "Both", "Off"};
      m.body += std::string(tr("Indicator: ")) + tr(kShown[static_cast<int>(battery_display_)]) + "\n" +
                (profile_.touch_screen ? tr("Tap to change") : profile_.talk_label + tr(" to change"));
    }
      break;
    case Menu::IdleTimer:
      m.detail = tr("Screen timeout");
      m.body = screen_timeout_ms_ ? std::to_string(screen_timeout_ms_ / 1000) + tr(" seconds\nDims halfway; first input wakes.")
                                 : tr("Always on\nSelect to enable automatic dimming and screen sleep.");
      break;
    case Menu::PowerOff:
      m.detail = tr("Power off");
      m.body = tr("Shut down the board. Use its PWR key to turn it on again.");
      break;
    case Menu::WifiSetup:
      m.detail = tr("Wi-Fi setup");
      m.body = tr("Start a temporary network to configure this device with your phone.");
      break;
    case Menu::Back:
      m.detail = tr("Back to Hermes");
      m.body = tr("Select to close settings.");
      break;
    case Menu::Closed: break;
  }
  if (!check_result_.empty()) m.body += "\n" + check_result_;
}

bool App::start_wifi_setup() {
  if (!on_wifi_setup || prompt_showing() || ota_busy() || ota_ == Ota::Restarting) return false;
  quick_controls_open_ = false;
  if (mode_ == Mode::Listening) cancel_listening("Wi-Fi setup");
  else if (mode_ != Mode::Idle) cancel_turn();
  stop_playback();
  close_settings();
  wake_display();
  talk_held_ = false;
  cancel_held_ = false;
  settings_chord_fired_ = false;
  wake_buttons_ = 0;
  wifi_setup_text_ = on_wifi_setup();
  // Encode the code once here: the model is rebuilt on every frame.
  wifi_setup_qr_ = {};
  if (on_wifi_setup_ap) {
    const WifiSetupAp ap = on_wifi_setup_ap();
    if (!ap.ssid.empty()) wifi_setup_qr_ = qr::encode(qr::payload(ap.ssid, ap.password));
  }
  if (wifi_setup_text_.empty()) set_hint_flash(tr("Wi-Fi setup unavailable; use USB"));
  update_model();
  return !wifi_setup_text_.empty();
}

void App::close_wifi_setup() {
  if (wifi_setup_text_.empty()) return;
  wifi_setup_text_.clear();
  wifi_setup_qr_ = {};
  if (on_wifi_setup_close) on_wifi_setup_close();
  update_model();
}

}  // namespace hg
