#include "hg/app.hpp"

#include <algorithm>
#include <vector>

namespace hg {
bool App::open_quick_controls() {
  if (!profile_.touch_screen || !ui_ || settings_open() || prompt_showing() || ota_busy() ||
      ota_ == Ota::Restarting || wifi_setup_open() || mode_ == Mode::Listening) return false;
  wake_display();
  talk_held_ = cancel_held_ = talk_release_pending_ = false;
  quick_controls_open_ = true;
  update_model();
  return true;
}

void App::close_quick_controls() {
  quick_controls_open_ = false;
  page_at_ = now() + page_dwell_ms();
  update_model();
}

QuickControl App::quick_control_hit(int x, int y) const {
  return ui_ && !display_sleeping_ ? ui_->control_hit(model_, x, y) : QuickControl::None;
}

void App::on_quick_control(QuickControl control) {
  if (!quick_controls_open_ || model_.screen != Screen::Controls || wake_display()) return;
  switch (control) {
    case QuickControl::VolumeDown:
    case QuickControl::VolumeUp:
      if (hal_.speaker)
        console("set volume " + std::to_string(std::max(0, std::min(100, int(volume_) +
                   (control == QuickControl::VolumeDown ? -10 : 10)))));
      break;
    case QuickControl::Mute:
      if (hal_.speaker) console(muted_ ? "set mute 0" : "set mute 1");
      break;
    case QuickControl::TextNormal: console("set text_size normal"); break;
    case QuickControl::TextLarge: console("set text_size large"); break;
    case QuickControl::Close: close_quick_controls(); break;
    case QuickControl::None: break;
  }
}

bool App::play_recording_cue(bool start) {
  if (!recording_cues_ || muted_ || !volume_ || !hal_.speaker ||
      profile_.speaker_rate < 1000 || profile_.speaker_rate > 96000) return false;
  if (!hal_.speaker->begin(profile_.speaker_rate)) return false;
  // A quiet 60 ms local tone. The start tone is excluded from transmitted mic
  // samples until the speaker drains; cancellation aborts it immediately.
  std::vector<int16_t> tone(profile_.speaker_rate * 60 / 1000);
  const uint32_t half_period = std::max<uint32_t>(1, profile_.speaker_rate / (start ? 1760 : 1320));
  for (size_t i = 0; i < tone.size(); ++i) {
    const int envelope = static_cast<int>(std::min(i, tone.size() - i - 1) * 700 / (tone.size() / 2));
    tone[i] = static_cast<int16_t>((i / half_period) % 2 ? envelope : -envelope);
  }
  hal_.speaker->write(tone.data(), tone.size());
  hal_.speaker->end();
  return true;
}
}  // namespace hg
