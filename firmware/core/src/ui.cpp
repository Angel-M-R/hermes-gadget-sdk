#include "hg/ui.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <vector>

#include "hg/font.hpp"
#include "hg/text.hpp"

namespace hg {
namespace {

// A backlit LCD shows a dark tint; an emissive panel keeps those pixels off (Ui::bg_, Ui::bar_).
constexpr uint16_t kTintBg = rgb565(10, 14, 20);
constexpr uint16_t kTintBar = rgb565(24, 31, 42);
constexpr uint16_t kButton = rgb565(24, 31, 42);  // the speaker button stays visible on black
constexpr uint16_t kText = rgb565(232, 238, 242);
constexpr uint16_t kDim = rgb565(132, 146, 160);
constexpr uint16_t kFaint = rgb565(52, 62, 76);
constexpr uint16_t kAccent = rgb565(242, 179, 61);
constexpr uint16_t kGreen = rgb565(61, 214, 140);
constexpr uint16_t kBlue = rgb565(90, 169, 242);
constexpr uint16_t kRed = rgb565(242, 95, 92);
constexpr uint16_t kYellow = rgb565(242, 201, 76);
constexpr uint16_t kGreenDim = rgb565(28, 92, 62);
constexpr uint16_t kBlueDim = rgb565(52, 96, 140);
constexpr uint16_t kAccentDim = rgb565(110, 82, 30);
constexpr uint16_t kRedDim = rgb565(122, 44, 44);

class Hash {
 public:
  Hash& add(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
      h_ ^= p[i];
      h_ *= 16777619u;
    }
    return *this;
  }
  Hash& add(const std::string& s) {
    add(s.data(), s.size());
    uint8_t sep = 0xFF;
    return add(&sep, 1);
  }
  template <typename T>
  Hash& val(T v) {
    return add(&v, sizeof(v));
  }
  uint32_t get() const { return h_; }

 private:
  uint32_t h_ = 2166136261u;
};

std::string fit(std::string_view s, int max_chars) {
  if (max_chars <= 0) return {};
  if (static_cast<int>(text_length(s)) <= max_chars) return std::string(s);
  if (max_chars <= 2) return std::string(text_prefix(s, static_cast<size_t>(max_chars)));
  return std::string(text_prefix(s, static_cast<size_t>(max_chars - 2))) + "..";
}

int cols_for(int width_px, int scale) { return width_px / (font::kCellWidth * scale); }

bool animated(Screen s) {
  switch (s) {
    case Screen::Boot:
    case Screen::Connecting:
    case Screen::Updating:
    case Screen::Ready:
    case Screen::Listening:
    case Screen::Thinking:
    case Screen::Responding:
      return true;
    default:
      return false;
  }
}

// Triangle wave in [0, period/2] for gentle pulsing.
int tri(uint32_t frame, int period) {
  int p = static_cast<int>(frame % static_cast<uint32_t>(period));
  return p < period / 2 ? p : period - p;
}

mascot::Frame mascot_frame(const UiModel& m) {
  if (m.screen == Screen::Responding && m.speaking) {
    return (m.frame / 2) % 2 ? mascot::Frame::Talk : mascot::Frame::Idle;
  }
  return m.frame % 45 < 2 ? mascot::Frame::Blink : mascot::Frame::Idle;  // blink every ~4.5 s
}

// Everything that changes hero pixels between frames, and nothing else, so
// a still mascot never triggers a redraw.
uint32_t hero_anim_key(const UiModel& m) {
  Hash h;
  h.val(mascot_frame(m));
  switch (m.screen) {
    case Screen::Listening: h.val(m.frame % 3).val(m.level / 20); break;
    case Screen::Thinking:
    case Screen::Connecting:
    case Screen::Updating: h.val((m.frame / 2) % 4); break;
    case Screen::Responding: h.val((m.frame / 2) % 3); break;
    default: break;
  }
  return h.get();
}

// Small icons as pixel rows ('#' = set), drawn at an integer scale.
constexpr const char* kSpeakerOn[] = {
    "....#...#..",
    "...##....#.",
    "..###.#...#",
    "#####..#..#",
    "#####..#..#",
    "#####..#..#",
    "..###.#...#",
    "...##....#.",
    "....#...#..",
};
constexpr const char* kSpeakerMuted[] = {
    "....#......",
    "...##......",
    "..###.#...#",
    "#####..#.#.",
    "#####...#..",
    "#####..#.#.",
    "..###.#...#",
    "...##......",
    "....#......",
};
constexpr const char* kMic[] = {
    "..###..",
    ".#####.",
    ".#####.",
    ".#####.",
    ".#####.",
    "#.###.#",
    "#.....#",
    ".#...#.",
    "..###..",
    "...#...",
    ".#####.",
};
constexpr const char* kPower[] = {
    "....#....",
    "..#.#.#..",
    ".#..#..#.",
    "#...#...#",
    "#.......#",
    "#.......#",
    ".#.....#.",
    "..#####..",
};

template <size_t N>
constexpr int glyph_w(const char* const (&rows)[N]) {
  return static_cast<int>(std::char_traits<char>::length(rows[0]));
}

template <size_t N>
void glyph(Canvas& c, int x, int y, const char* const (&rows)[N], int scale, uint16_t color) {
  for (size_t r = 0; r < N; ++r) {
    for (int col = 0; rows[r][col]; ++col) {
      if (rows[r][col] == '#') c.fill_rect(x + col * scale, y + static_cast<int>(r) * scale, scale, scale, color);
    }
  }
}

// How far a rounded corner of radius r eats into a pixel row d pixels from the
// top or bottom edge.
int corner_cut(int r, int d) {
  if (r <= 0 || d >= r) return 0;
  const int k = r - d, target = r * r - k * k;
  int x = 0;
  while ((x + 1) * (x + 1) <= target) ++x;  // integer square root: no floats in the renderer
  return r - x;
}

// Screens with the speaker button and the key icons: the conversation itself.
bool conversation(Screen s) {
  return s == Screen::Ready || s == Screen::Listening || s == Screen::Thinking || s == Screen::Responding;
}

uint16_t caption_color(Screen s) {
  switch (s) {
    case Screen::Listening: return kGreen;
    case Screen::Thinking: return kBlue;
    case Screen::Responding:
    case Screen::Connecting:
    case Screen::Updating:
    case Screen::Pairing:
    case Screen::Prompt: return kAccent;
    case Screen::Offline:
    case Screen::Error: return kRed;
    default: return kText;
  }
}

}  // namespace

const char* screen_name(Screen s) {
  switch (s) {
    case Screen::Boot: return "boot";
    case Screen::Offline: return "offline";
    case Screen::Connecting: return "connecting";
    case Screen::Pairing: return "pairing";
    case Screen::Ready: return "ready";
    case Screen::Listening: return "listening";
    case Screen::Thinking: return "thinking";
    case Screen::Responding: return "responding";
    case Screen::Card: return "card";
    case Screen::Image: return "image";
    case Screen::Error: return "error";
    case Screen::Prompt: return "prompt";
    case Screen::Updating: return "updating";
    case Screen::Settings: return "settings";
    case Screen::Setup: return "setup";
    case Screen::Controls: return "controls";
  }
  return "unknown";
}

Ui::Ui(Display& display, bool touch_screen) : display_(display), panel_(display.info()), info_(panel_) {
  bg_ = panel_.emissive ? rgb565(0, 0, 0) : kTintBg;
  bar_ = panel_.emissive ? rgb565(0, 0, 0) : kTintBar;
  if (panel_.round) {
    // The largest square inside the circle; the corners of the panel do not exist.
    const int side = std::min(panel_.width, panel_.height) * 707 / 1000;
    ox_ = (panel_.width - side) / 2;
    oy_ = (panel_.height - side) / 2;
    info_.width = info_.height = static_cast<uint16_t>(side);
  }
  int w = info_.width, h = info_.height;
  int s = std::max(1, std::min(4, std::min(w / 160, h / 120)));
  layout_.scale = s;
  layout_.body_scale = s;
  // Rounded glass corners: the bars grow and their text moves towards the middle, clear of the curve.
  const int corner = panel_.round ? 0 : panel_.corner_radius;
  bar_pad_ = corner ? 3 * s : 0;
  layout_.top_h = Canvas::line_height(s) + 2 * s + bar_pad_;
  layout_.bottom_h = layout_.top_h;
  if (panel_.round) {
    const int dot_r = std::max(2, 3 * s / 2 + 1);
    layout_.top_h = std::max(36, 2 * dot_r + 14 * s);
  }
  layout_.title_w = panel_.round ? std::min(w, 120 * s) : w;
  if (touch_screen) layout_.bottom_h = std::max(layout_.bottom_h, std::min(40, h / 4));
  if (corner) {
    top_inset_ = corner_cut(corner, s + bar_pad_) + s;
    bottom_inset_ = corner_cut(corner, layout_.bottom_h - s - font::kGlyphHeight * s) + s;
  }
  layout_.header_h = Canvas::line_height(s + 1) + 4 * s;
  layout_.main_y = layout_.top_h;
  layout_.main_h = h - layout_.top_h - layout_.bottom_h;
  int margin = 4 * s;
  int content_h = h - layout_.top_h - layout_.header_h - layout_.bottom_h;
  layout_.body_cols = cols_for(w - 2 * margin, s);
  layout_.body_rows = std::max(1, (content_h - margin) / Canvas::line_height(s));

  // Hero layout: the mascot is sized per screen (see hero_geom).
  layout_.hero_cols = cols_for(w - 4 * s, s);
}

void Ui::invalidate() { valid_ = false; }

void Ui::set_large_text(bool large) {
  const int s = layout_.scale;
  layout_.body_scale = s + (large ? 1 : 0);
  const int content_h = info_.height - layout_.top_h - layout_.header_h - layout_.bottom_h;
  layout_.body_cols = cols_for(info_.width - 8 * s, layout_.body_scale);
  layout_.body_rows = std::max(1, (content_h - 4 * s) / Canvas::line_height(layout_.body_scale));
  invalidate();
}

Canvas Ui::canvas() {
  return Canvas(display_.framebuffer() + oy_ * panel_.width + ox_, info_.width, info_.height, info_.swap_bytes,
                panel_.width);
}

void Ui::flush(int y0, int y1) {
  display_.flush(static_cast<uint16_t>(y0 + oy_), static_cast<uint16_t>(y1 + oy_));
}

void Ui::render(const UiModel& m) {
  const int h = info_.height;
  if (!valid_ && (ox_ || oy_)) {
    // Round panel: everything outside the UI area stays the background colour.
    Canvas panel(display_.framebuffer(), panel_.width, panel_.height, panel_.swap_bytes);
    panel.fill_rect(0, 0, panel_.width, panel_.height, bg_);
    display_.flush(0, panel_.height);
  }
  if (panel_.round) draw_round_battery(m);
  Canvas c = canvas();
  const int y_header = layout_.top_h;
  const int y_content = y_header + layout_.header_h;
  const int y_bottom = h - layout_.bottom_h;

  const bool speaker = m.speaker_button && conversation(m.screen);
  speaker_ = speaker ? (m.hero ? hero_speaker(m) : header_speaker(m)) : Circle{};
  speaker_in_header_ = speaker && !m.hero;

  uint32_t hashes[4];
  hashes[0] = Hash().add(m.title).val(m.link).val(m.battery).val(m.battery_charging)
                  .val(m.battery_icon).val(m.battery_text).val(m.battery_low).val(m.usb_power).val(m.language)
                  .val(panel_.round && m.screen == Screen::Settings).val(panel_.round && m.settings_hold).get();
  hashes[1] = Hash()
                  .val(m.screen)
                  .add(m.headline)
                  .val(animated(m.screen) ? m.frame : 0u)
                  .val(m.screen == Screen::Listening ? m.level : uint8_t(0))
                  .val(m.speaking)
                  .val(speaker)
                  .val(m.muted)
                  .val(m.speaker_pressed)
                  .get();
  hashes[2] =
      Hash().val(m.screen).add(m.detail).add(m.body).val(m.qr ? m.qr->size : 0).add(m.code).val(m.scroll).val(m.color_test)
            .val(m.volume).val(m.muted).val(m.large_text).val(m.language).get();
  hashes[3] = Hash().add(m.hint).val(m.reading).val(m.screen).val(m.language)
                  .val(m.reading != Reading::None ? hashes[2] : 0u).get();

  if (m.hero) {
    // Hero mode: top bar, one mascot band, bottom bar.
    for (int i : {0, 3}) {
      if (valid_ && hash_[i] == hashes[i]) continue;
      int y0 = i == 0 ? 0 : y_bottom, y1 = i == 0 ? y_header : h;
      c.set_clip_rows(y0, y1);
      if (i == 0) draw_top(c, m);
      else draw_bottom(c, m);
      flush(y0, y1);
      hash_[i] = hashes[i];
    }
    uint32_t stat = Hash()
                        .val(m.screen)
                        .add(m.headline)
                        .add(m.detail)
                        .add(m.yes)
                        .add(m.no)
                        .val(m.caption_lines)
                        .val(speaker)
                        .val(m.muted)
                        .val(m.speaker_pressed)
                        .val(m.talk_edge)
                        .val(m.talk_dy)
                        .val(m.power_edge)
                        .val(m.power_dy)
                        .get();
    uint32_t anim = hero_anim_key(m);
    int y0 = y_header, y1 = y_bottom;
    bool redraw = !valid_ || !hero_valid_ || stat != hero_static_;
    if (!redraw && anim != hero_anim_) {
      hero_anim_rows(m, y0, y1);  // only what can move
      redraw = true;
    }
    if (redraw) {
      c.set_clip_rows(y0, y1);
      draw_hero(c, m);
      flush(y0, y1);
    }
    hero_static_ = stat;
    hero_anim_ = anim;
    hero_valid_ = true;
    hash_[1] = hash_[2] = 0;  // header and content must repaint when hero mode ends
    valid_ = true;
    return;
  }
  hero_valid_ = false;

  struct Band {
    int y0, y1;
    void (Ui::*draw)(Canvas&, const UiModel&);
  };
  const Band bands[4] = {
      {0, y_header, &Ui::draw_top},
      {y_header, y_content, &Ui::draw_header},
      {y_content, y_bottom, &Ui::draw_content},
      {y_bottom, h, &Ui::draw_bottom},
  };
  for (int i = 0; i < 4; ++i) {
    // An image owns the area between the bars; the app draws and flushes it.
    if (m.screen == Screen::Image && (i == 1 || i == 2)) {
      hash_[i] = 0;
      continue;
    }
    if (valid_ && hash_[i] == hashes[i]) continue;
    c.set_clip_rows(bands[i].y0, bands[i].y1);
    (this->*bands[i].draw)(c, m);
    flush(bands[i].y0, bands[i].y1);
    hash_[i] = hashes[i];
  }
  valid_ = true;
}

void Ui::draw_top(Canvas& c, const UiModel& m) {
  const int s = layout_.scale;
  const int w = info_.width;
  c.fill_rect(0, 0, w, layout_.top_h, panel_.round ? bg_ : bar_);
  uint16_t dot = kRed;
  const char* label = "OFFLINE";
  switch (m.link) {
    case Link::Offline: dot = kRed; label = "NO NET"; break;
    case Link::Network: dot = kYellow; label = "NET"; break;
    case Link::Connecting: dot = kYellow; label = "LINK"; break;
    case Link::Online: dot = kGreen; label = "ONLINE"; break;
  }
  label = localized(label, m.language);
  int ty = s + bar_pad_;
  if (panel_.round) {
    // The settings hold target: a dim label under the link dot, with no bar
    // behind it. The label line (2r + 4s .. 2r + 13s) fits the band.
    int r = std::max(2, 3 * s / 2 + 1);
    if (m.settings_hold) {
      const char* control = localized(m.screen == Screen::Settings ? "BACK TO HERMES" : "SETTINGS", m.language);
      c.text((w - Canvas::text_width(control, s)) / 2, 2 * r + 4 * s, control, s, kDim);
    }
    c.fill_circle(w / 2, s + r, r, dot);
    return;
  }
  const int pad = 3 * s + std::max<int>(panel_.corner_inset, top_inset_);
  int label_w = Canvas::text_width(label, s);
  int label_x = w - pad - label_w;
  c.text(label_x, ty, label, s, kDim);
  int r = std::max(2, 3 * s / 2 + 1);
  const int dot_x = label_x - 3 * s - r;
  const int text_mid = ty + font::kGlyphHeight * s / 2;
  c.fill_circle(dot_x, text_mid, r, dot);
  int title_end = dot_x - r;
  if (m.battery >= 0) {
    title_end -= 4 * s + battery_width(m);
    draw_battery(c, m, title_end, text_mid);
  } else if (m.usb_power) {
    title_end -= 4 * s + Canvas::text_width("USB", s);
    c.text(title_end, ty, "USB", s, kDim);
  }
  int title_cols = cols_for(title_end - 3 * s - pad, s);
  c.text(pad, ty, fit(m.title, title_cols), s, kText);
}

// Round panels: the battery is centred at the very top of the panel, above the UI
// square, as high as the circle leaves it room plus a small margin.
void Ui::draw_round_battery(const UiModel& m) {
  const int s = layout_.scale;
  const int bh = 7 * s;  // the taller of the icon (5s + 1) and the text (7s)
  const int hw = (m.battery >= 0 ? battery_width(m) : 0) / 2 + 2 * s;
  const int radius = panel_.width / 2;
  int y0 = 0;
  // The first row where the strip's top corners lie inside the circle.
  while (y0 < oy_ && (radius - y0) * (radius - y0) + hw * hw > radius * radius) ++y0;
  y0 = std::min(y0 + 3 * s, std::max(0, oy_ - bh));
  const int y1 = std::min<int>(oy_, y0 + bh);
  const uint32_t key = Hash().val(m.battery).val(m.battery_charging).val(m.battery_low).val(m.battery_icon)
                           .val(m.battery_text).val(y0).get();
  if (valid_ && key == battery_hash_) return;
  battery_hash_ = key;
  Canvas panel(display_.framebuffer(), panel_.width, panel_.height, panel_.swap_bytes);
  // Clear the band the strip can occupy at any level, so a shorter or hidden strip leaves nothing behind.
  panel.fill_rect(0, 0, panel_.width, oy_, bg_);
  if (m.battery >= 0) draw_battery(panel, m, (panel_.width - battery_width(m)) / 2, (y0 + y1) / 2);
  display_.flush(0, static_cast<uint16_t>(oy_));
}

int Ui::battery_width(const UiModel& m) const {
  const int s = layout_.scale;
  const int icon = m.battery_icon ? 10 * s : 0;
  const int text = m.battery_text ? Canvas::text_width(std::to_string(m.battery) + "%", s) : 0;
  return icon + text + (icon && text ? 2 * s : 0);
}

// A battery outline with a terminal nub, filled to the level, then the percent.
void Ui::draw_battery(Canvas& c, const UiModel& m, int x, int cy) {
  const int s = layout_.scale;
  if (m.battery_icon) {
    const int bw = 9 * s, bh = 5 * s + 1, t = std::max(1, s / 2);
    const int y = cy - bh / 2;
    const uint16_t fill = m.battery_charging ? kGreen : m.battery_low ? kRed : kText;
    c.fill_rect(x, y, bw, bh, kDim);
    c.fill_rect(x + t, y + t, bw - 2 * t, bh - 2 * t, panel_.round ? bg_ : bar_);
    c.fill_rect(x + bw, cy - s, s, 2 * s, kDim);
    const int inner = bw - 4 * t;
    const int level = std::max(m.battery > 0 ? 1 : 0, inner * std::min<int>(m.battery, 100) / 100);
    c.fill_rect(x + 2 * t, y + 2 * t, level, bh - 4 * t, fill);
    x += 12 * s;
  }
  if (m.battery_text) {
    // Without the icon, the text itself turns green while charging.
    const uint16_t col = m.battery_charging ? (m.battery_icon ? kDim : kGreen) : m.battery_low ? kRed : kDim;
    c.text(x, cy - (font::kGlyphHeight * s) / 2, std::to_string(m.battery) + "%", s, col);
  }
}

void Ui::draw_indicator(Canvas& c, const UiModel& m, int cx, int cy, int r) {
  const int s = layout_.scale;
  auto spinner = [&](uint16_t color) {
    int dot_r = std::max(1, r / 5);
    // Eight positions around the circle; precomputed unit vectors * 100.
    static const int kDx[8] = {0, 71, 100, 71, 0, -71, -100, -71};
    static const int kDy[8] = {-100, -71, 0, 71, 100, 71, 0, -71};
    int ring_r = r - dot_r;
    int head = static_cast<int>(m.frame % 8);
    for (int i = 0; i < 8; ++i) {
      int age = (head - i + 8) % 8;
      uint16_t col = age == 0 ? color : (age < 3 ? kDim : kFaint);
      c.fill_circle(cx + kDx[i] * ring_r / 100, cy + kDy[i] * ring_r / 100, dot_r, col);
    }
  };
  auto bars = [&](uint16_t color, int level_pct) {
    const int n = 5;
    int bw = std::max(3, (2 * r) / (2 * n - 1));
    int x0 = cx - (n * bw + (n - 1) * bw) / 2;
    for (int i = 0; i < n; ++i) {
      int jitter = static_cast<int>((m.frame * 3 + static_cast<uint32_t>(i) * 5) % 7);
      int pct = std::max(10, level_pct * (4 + jitter) / 10);
      int bh = std::max(2 * s, std::min(2 * r, 2 * r * pct / 100));
      c.fill_rect(x0 + i * 2 * bw, cy - bh / 2, bw, bh, color);
    }
  };

  switch (m.screen) {
    case Screen::Boot:
    case Screen::Connecting:
    case Screen::Updating:
      spinner(kAccent);
      break;
    case Screen::Thinking:
      spinner(kBlue);
      break;
    case Screen::Offline:
    case Screen::Error: {
      c.fill_circle(cx, cy, r, kRed);
      int tw = Canvas::text_width("!", s);
      c.text(cx - tw / 2, cy - (7 * s) / 2, "!", s, bg_);
      break;
    }
    case Screen::Pairing:
      c.ring(cx, cy, r, s + 1, kAccent);
      c.fill_circle(cx, cy, r / 3, kAccent);
      break;
    case Screen::Ready: {
      int pulse = tri(m.frame, 24);  // 0..12
      c.fill_circle(cx, cy, r * (8 + pulse / 3) / 12, kAccent);
      break;
    }
    case Screen::Listening:
      bars(kGreen, std::max<int>(m.level, 8));
      break;
    case Screen::Responding:
      if (m.speaking) {
        bars(kAccent, 40 + tri(m.frame, 8) * 10);
      } else {
        c.fill_circle(cx, cy, r * 2 / 3, kAccent);
      }
      break;
    case Screen::Card:
      c.fill_round_rect(cx - r, cy - r, 2 * r, 2 * r, r / 3, kAccent);
      break;
    case Screen::Prompt: {
      c.fill_circle(cx, cy, r, kAccent);
      int tw = Canvas::text_width("?", s);
      c.text(cx - tw / 2, cy - (7 * s) / 2, "?", s, bg_);
      break;
    }
    case Screen::Image:
    case Screen::Settings:
    case Screen::Setup:
    case Screen::Controls:
      break;
  }
}

void Ui::draw_header(Canvas& c, const UiModel& m) {
  const int s = layout_.scale;
  const int y0 = layout_.top_h;
  const int hh = layout_.header_h;
  c.fill_rect(0, y0, info_.width, hh, bg_);
  int r = hh / 2 - 2 * s;
  int cx = 4 * s + r;
  int cy = y0 + hh / 2;
  draw_indicator(c, m, cx, cy, r);
  int tx = cx + r + 4 * s;
  int hs = s + 1;
  int right = info_.width - 2 * s;
  if (speaker_.r) {
    draw_speaker(c, m, speaker_);
    right = speaker_.cx - speaker_.r - 2 * s;
  }
  int cols = cols_for(right - tx, hs);
  c.text(tx, cy - (font::kGlyphHeight * hs) / 2, fit(m.headline, cols), hs, kText);
  c.fill_rect(4 * s, y0 + hh - 1, info_.width - 8 * s, 1, kFaint);
}

int Ui::body_rows(const UiModel& m) const {
  int rows = layout_.body_rows;
  if (m.detail.empty()) return rows;
  int detail = static_cast<int>(wrap_text(m.detail, layout_.body_cols).size());
  int max_detail = m.body.empty() ? rows : std::min(2, rows - 1);
  return rows - std::max(0, std::min(detail, max_detail));
}

void Ui::draw_content(Canvas& c, const UiModel& m) {
  const int s = layout_.body_scale;
  const int w = info_.width;
  const int y0 = layout_.top_h + layout_.header_h;
  const int y1 = info_.height - layout_.bottom_h;
  const int margin = 4 * layout_.scale;
  const int lh = Canvas::line_height(s);
  c.fill_rect(0, y0, w, y1 - y0, bg_);
  int y = y0 + margin;
  if (m.screen == Screen::Controls) { draw_controls(c, m); return; }

  if (m.color_test) {
    const uint16_t colors[] = {rgb565(255, 0, 0), rgb565(0, 255, 0), rgb565(0, 0, 255),
                               rgb565(255, 255, 255), rgb565(0, 0, 0)};
    for (int i = 0; i < 5; ++i) {
      const int left = w * i / 5, right = w * (i + 1) / 5;
      c.fill_rect(left, y0, right - left, y1 - y0, colors[i]);
    }
    return;
  }

  if (m.qr && m.qr->ok() && draw_qr(c, m, y0, y1)) return;

  if (m.screen == Screen::Boot) {
    int big = s + 2;
    const char* name = "HERMES";
    int cy = (y0 + y1) / 2 - Canvas::line_height(big) / 2;
    c.text((w - Canvas::text_width(name, big)) / 2, cy, name, big, kAccent);
    int sy = cy + Canvas::line_height(big);
    c.text((w - Canvas::text_width("gadget", s)) / 2, sy, "gadget", s, kDim);
    if (!m.detail.empty()) {
      std::string d = fit(m.detail, layout_.body_cols);
      c.text((w - Canvas::text_width(d, s)) / 2, y1 - lh, d, s, kFaint);
    }
    return;
  }

  int rows = body_rows(m);
  if (!m.detail.empty()) {
    uint16_t dcol = m.screen == Screen::Error || m.screen == Screen::Offline ? kRed : kDim;
    if (m.screen == Screen::Thinking) dcol = kBlue;
    auto detail_lines = wrap_text(m.detail, layout_.body_cols);
    int shown = layout_.body_rows - rows;
    for (int i = 0; i < shown; ++i) {
      c.text(margin, y, detail_lines[static_cast<size_t>(i)], s, dcol);
      y += lh;
    }
  }
  if (m.body.empty() || rows <= 0) return;

  auto lines = wrap_text(m.body, layout_.body_cols);
  int total = static_cast<int>(lines.size());
  int max_first = std::max(0, total - rows);
  int first = m.scroll < 0 ? max_first : std::min(m.scroll, max_first);
  uint16_t col = m.screen == Screen::Ready ? kDim : kText;
  for (int i = 0; i < rows && first + i < total; ++i) {
    c.text(margin, y + i * lh, lines[static_cast<size_t>(first + i)], s, col);
  }
  if (total > rows) {
    // Scroll indicator along the right edge.
    int track_y = y, track_h = rows * lh;
    int thumb_h = std::max(2 * s, track_h * rows / total);
    int thumb_y = track_y + (track_h - thumb_h) * first / std::max(1, max_first);
    c.fill_rect(w - 2 * s, track_y, s, track_h, kFaint);
    c.fill_rect(w - 2 * s, thumb_y, s, thumb_h, kDim);
  }
}

// A Wi-Fi setup code: a white panel with a four-module quiet zone, one square
// per module, at the largest integer scale that fits the content band. A phone
// camera needs the quiet zone and dark-on-light contrast, so neither is optional.
// Returns false, drawing nothing, when the code does not fit beside the text.
bool Ui::draw_qr(Canvas& c, const UiModel& m, int y0, int y1) {
  const int n = m.qr->size;
  const int w = info_.width;
  const int s = layout_.scale;
  const int lh = Canvas::line_height(s);
  const int margin = 4 * s;
  const int pad = 4;                     // quiet zone, in modules
  const int cells = n + 2 * pad;         // the code plus its quiet zone
  const int avail_h = y1 - y0 - 2 * margin;

  // The code only joins the network: the phone still needs the address, and a
  // phone that cannot scan needs the name and password. So every line of the
  // instructions stays on screen, and the code is shown only where it fits
  // beside them at kMinScale or larger, below which it is not reliably
  // scannable. Otherwise the caller draws the text-only screen.
  constexpr int kMinScale = 3;
  const std::vector<std::string> lines = wrap_text(m.body, layout_.body_cols);
  const int text_rows = static_cast<int>(lines.size());
  const int reserve = text_rows ? text_rows * lh + 2 * s : 0;
  const int scale = std::min((w - 2 * margin) / cells, (avail_h - reserve) / cells);
  if (scale < kMinScale) return false;

  const int side = cells * scale;
  const int x0 = (w - side) / 2;
  const int block = side + reserve;
  const int py0 = y0 + margin + std::max(0, (avail_h - block) / 2);
  c.fill_rect(x0, py0, side, side, rgb565(255, 255, 255));
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      if (m.qr->at(x, y)) {
        c.fill_rect(x0 + (x + pad) * scale, py0 + (y + pad) * scale, scale, scale, rgb565(0, 0, 0));
      }
    }
  }
  int ty = py0 + side + 2 * s;
  for (int i = 0; i < text_rows; ++i) {
    c.text((w - Canvas::text_width(lines[static_cast<size_t>(i)], s)) / 2, ty, lines[static_cast<size_t>(i)], s, kDim);
    ty += lh;
  }
  return true;
}

Ui::HeroGeom Ui::hero_geom(const UiModel& m) const {
  HeroGeom g;
  const int s = layout_.scale, w = info_.width, lh = Canvas::line_height(s);
  const int cols = layout_.hero_cols;
  if (!m.detail.empty()) g.detail = wrap_text(m.detail, cols);
  const int buttons_h = m.yes.empty() && m.no.empty() ? 0 : lh + 6 * s;
  auto caption_h = [&](int lines) { return (1 + lines) * lh + s; };
  // Never shrink the mascot below the smallest bitmap: drop caption lines instead.
  const int smallest = mascot::pick(0, mascot::Frame::Idle)->size;
  int max_lines = std::max(1, static_cast<int>(m.caption_lines));
  while (max_lines > 1 && layout_.main_h - caption_h(max_lines) - buttons_h - 4 * s < smallest) --max_lines;
  if (static_cast<int>(g.detail.size()) > max_lines) {
    g.detail.resize(static_cast<size_t>(max_lines));
    std::string& last = g.detail.back();
    if (static_cast<int>(last.size()) + 2 > cols) last.resize(static_cast<size_t>(std::max(0, cols - 2)));
    last += "..";
  }
  // Reserve one detail line even when there is none, so the mascot keeps its size.
  const int ch = caption_h(std::max<int>(1, static_cast<int>(g.detail.size())));
  const int avail = std::min(w - 8 * s, layout_.main_h - ch - buttons_h - 4 * s);
  g.size = mascot::pick(avail, mascot::Frame::Idle)->size;
  const int block = g.size + 2 * s + ch + buttons_h;
  g.x = (w - g.size) / 2;
  g.y = layout_.main_y + std::max(s, (layout_.main_h - block) / 2);
  g.caption_y = g.y + g.size + 2 * s;
  g.buttons_y = g.caption_y + ch + 2 * s;
  return g;
}

void Ui::hero_anim_rows(const UiModel& m, int& y0, int& y1) const {
  const HeroGeom g = hero_geom(m);
  const int size = g.size, my = g.y;
  const mascot::Anchors& a = mascot::anchors();
  int top = my + size * a.eyes_y0 / 1000, bottom = my + size * a.eyes_y1 / 1000;
  auto add = [&](int lo, int hi) {
    top = std::min(top, lo);
    bottom = std::max(bottom, hi);
  };
  const int reach = size * 280 / 1000;  // outermost wave radius + stroke
  switch (m.screen) {
    case Screen::Listening: {
      int ey = my + size * a.ear_cup.y / 1000;
      add(ey - reach, ey + reach);
      break;
    }
    case Screen::Responding: {
      int mouth = my + size * a.mouth.y / 1000;
      add(my + size * a.mouth_y0 / 1000, my + size * a.mouth_y1 / 1000);
      add(mouth - reach, mouth + reach);
      break;
    }
    case Screen::Thinking:
    case Screen::Connecting:
    case Screen::Updating: {
      int ty = my + size * a.head_top_right.y / 1000;
      add(ty - size / 6, ty + size / 12);
      break;
    }
    default: break;
  }
  y0 = std::max(top - 1, layout_.top_h);
  y1 = std::min(bottom + 2, static_cast<int>(info_.height) - layout_.bottom_h);
}

void Ui::draw_hero(Canvas& c, const UiModel& m) {
  const int s = layout_.scale;
  const int w = info_.width;
  const int y0 = layout_.top_h;
  const int y1 = info_.height - layout_.bottom_h;
  const int lh = Canvas::line_height(s);
  c.fill_rect(0, y0, w, y1 - y0, bg_);

  const HeroGeom g = hero_geom(m);
  const int size = g.size, mx = g.x, my = g.y;
  const mascot::Anchors& a = mascot::anchors();
  if (const mascot::Bitmap* b = mascot::pick(size, mascot_frame(m))) {
    c.bitmap1(mx, my, b->size, b->size, b->bits, kText);
  }
  const int stroke = std::max(2, size / 48);
  auto waves = [&](int cx, int cy, int dir, int lit, int phase, uint16_t on, uint16_t dim, int r0) {
    for (int i = 0; i < 3; ++i) {
      int r = size * (r0 + 60 * i) / 1000;
      uint16_t col = i >= lit ? kFaint : (i == phase ? on : dim);
      c.arc(cx, cy, r, stroke, dir, col);
    }
  };
  auto dots = [&](uint16_t on, uint16_t dim) {
    int r = std::max(2, size / 30);
    int gap = 3 * r;
    int tx = mx + size * a.head_top_right.x / 1000;
    int ty = my + size * a.head_top_right.y / 1000;
    int active = static_cast<int>((m.frame / 2) % 4);
    for (int i = 0; i < 3; ++i) {
      c.fill_circle(tx + i * gap, ty - i * gap / 2, r, i == active ? on : dim);
    }
  };
  switch (m.screen) {
    case Screen::Listening: {
      int lit = 1 + std::min(2, m.level / 34);
      waves(mx + size * a.ear_cup.x / 1000, my + size * a.ear_cup.y / 1000, a.listen_dir, lit,
            static_cast<int>(m.frame % 3), kGreen, kGreenDim, 90);
      break;
    }
    case Screen::Responding:
      if (m.speaking) {
        // Start outside the face outline so the waves read as sound, not as lines on her cheek.
        waves(mx + size * a.mouth.x / 1000, my + size * a.mouth.y / 1000, a.talk_dir, 3,
              static_cast<int>((m.frame / 2) % 3), kAccent, kAccentDim, 130);
      }
      break;
    case Screen::Thinking: dots(kBlue, kBlueDim); break;
    case Screen::Connecting:
    case Screen::Updating: dots(kAccent, kAccentDim); break;
    default: break;
  }
  if (speaker_.r) draw_speaker(c, m, speaker_);
  if (conversation(m.screen)) draw_key_marks(c, m, g);

  // Caption: headline, then the detail lines, centred.
  const int cols = layout_.hero_cols;
  std::string head = fit(m.headline, cols);
  int cy = g.caption_y;
  c.text((w - Canvas::text_width(head, s)) / 2, cy, head, s, caption_color(m.screen));
  for (const auto& line : g.detail) {
    cy += lh;
    c.text((w - Canvas::text_width(line, s)) / 2, cy, line, s, kDim);
  }

  // Answer buttons (Prompt screen): TALK on the left, CANCEL on the right.
  if (!m.yes.empty() || !m.no.empty()) {
    const int bh = lh + 4 * s, gap = 4 * s, margin = 4 * s;
    const int bw = (w - 2 * margin - gap) / 2;
    auto button = [&](int x, const std::string& label, uint16_t fill) {
      c.fill_round_rect(x, g.buttons_y, bw, bh, 3 * s, fill);
      std::string t = fit(label, cols_for(bw - 2 * s, s));
      c.text(x + (bw - Canvas::text_width(t, s)) / 2, g.buttons_y + 2 * s + s / 2, t, s, kText);
    };
    if (!m.yes.empty()) button(margin, m.yes, kGreenDim);
    if (!m.no.empty()) button(margin + bw + gap, m.no, kRedDim);
  }
}

Ui::Circle Ui::hero_speaker(const UiModel&) const {
  const int s = layout_.scale;
  const int r = 18 * s;  // a thumb-sized target: 72 px across at scale 2
  return {4 * s + r, layout_.top_h + 4 * s + r, r};
}

Ui::Circle Ui::header_speaker(const UiModel&) const {
  const int s = layout_.scale;
  const int r = layout_.header_h / 2 - 2 * s;  // the same size as the indicator on the left
  return {info_.width - 4 * s - r, layout_.top_h + layout_.header_h / 2, r};
}

bool Ui::speaker_hit(int x, int y) const {
  if (!speaker_.r) return false;
  const int lx = x - ox_, ly = y - oy_;
  const int reach = speaker_.r + 6 * layout_.scale;  // fingers are wider than the icon
  // Never steal the title bar (holding it opens the settings), nor the text under the header.
  if (ly < layout_.top_h || (speaker_in_header_ && ly >= layout_.top_h + layout_.header_h)) return false;
  return std::abs(lx - speaker_.cx) <= reach && std::abs(ly - speaker_.cy) <= reach;
}

void Ui::draw_speaker(Canvas& c, const UiModel& m, Circle at) {
  // Muted stands out, like a lit button; on is quiet.
  c.fill_circle(at.cx, at.cy, at.r, m.muted ? kText : kButton);
  if (m.speaker_pressed) c.ring(at.cx, at.cy, at.r, std::max(2, layout_.scale), kAccent);
  const int gw = glyph_w(kSpeakerOn), gh = static_cast<int>(std::size(kSpeakerOn));
  const int gs = std::max(1, at.r * 6 / 5 / gw);
  const int gx = at.cx - gw * gs / 2, gy = at.cy - gh * gs / 2;
  if (m.muted) glyph(c, gx, gy, kSpeakerMuted, gs, bg_);
  else glyph(c, gx, gy, kSpeakerOn, gs, kDim);
}

void Ui::draw_key_marks(Canvas& c, const UiModel& m, const HeroGeom& g) {
  const int s = layout_.scale, w = info_.width, lh = Canvas::line_height(s);
  // The caption lines under the mascot, centred: an icon never sits on one.
  const std::string head = fit(m.headline, layout_.hero_cols);
  auto crosses_caption = [&](int x0, int y0, int x1, int y1) {
    for (size_t i = 0; i <= g.detail.size(); ++i) {
      const int tw = Canvas::text_width(i == 0 ? head : g.detail[i - 1], s);
      const int ly = g.caption_y + static_cast<int>(i) * lh;
      if (y0 < ly + lh && y1 > ly && x0 < (w + tw) / 2 + s && x1 > (w - tw) / 2 - s) return true;
    }
    return false;
  };
  auto place = [&](char edge, int dy, int gw, int gh, int& x, int& y) {
    if (edge != 'l' && edge != 'r') return false;
    x = edge == 'r' ? w - 3 * s - gw * s : 3 * s;
    y = info_.height / 2 + dy - gh * s / 2;
    return !crosses_caption(x, y, x + gw * s, y + gh * s);
  };
  int x = 0, y = 0;
  if (place(m.talk_edge, m.talk_dy, glyph_w(kMic), static_cast<int>(std::size(kMic)), x, y))
    glyph(c, x, y, kMic, s, m.screen == Screen::Listening ? kGreen : kDim);
  if (place(m.power_edge, m.power_dy, glyph_w(kPower), static_cast<int>(std::size(kPower)), x, y))
    glyph(c, x, y, kPower, s, kDim);
}

int Ui::reading_inset() const {
  return std::max(2 * layout_.scale + bottom_inset_, panel_.round ? 0 : int(panel_.corner_radius));
}

Ui::Rect Ui::control_rect(QuickControl control) const {
  const int s = layout_.scale, inset = reading_inset(), width = info_.width - 2 * inset;
  if (control == QuickControl::Close)
    return {inset, info_.height - layout_.bottom_h, width, layout_.bottom_h};
  const int top = layout_.top_h + layout_.header_h + 2 * s;
  const int row = (info_.height - layout_.bottom_h - top - 2 * s) / 2;
  const bool text = control == QuickControl::TextNormal || control == QuickControl::TextLarge;
  const int column = control == QuickControl::VolumeUp ? 2
                     : control == QuickControl::Mute || control == QuickControl::TextLarge ? 1 : 0;
  // Leave room for the mute label in both languages; +/- need less width.
  const int audio_edges[] = {0, 1, 4, 5};
  const int left = inset + width * (text ? column : audio_edges[column]) / (text ? 2 : 5);
  const int right = inset + width * (text ? column + 1 : audio_edges[column + 1]) / (text ? 2 : 5);
  return {left, top + (text ? row : 0) + Canvas::line_height(s), right - left,
          std::max(1, row - Canvas::line_height(s) - 2 * s)};
}

QuickControl Ui::control_hit(const UiModel& m, int x, int y) const {
  if (m.screen != Screen::Controls) return QuickControl::None;
  x -= ox_; y -= oy_;
  for (auto control : {QuickControl::VolumeDown, QuickControl::Mute, QuickControl::VolumeUp,
                       QuickControl::TextNormal, QuickControl::TextLarge, QuickControl::Close}) {
    if (!m.speaker_button && (control == QuickControl::VolumeDown || control == QuickControl::VolumeUp ||
                             control == QuickControl::Mute)) continue;
    const auto r = control_rect(control);
    if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return control;
  }
  return QuickControl::None;
}

void Ui::draw_controls(Canvas& c, const UiModel& m) {
  const int s = layout_.scale, top = layout_.top_h + layout_.header_h + 2 * s;
  const int row = (info_.height - layout_.bottom_h - top - 2 * s) / 2;
  const std::string volume = std::string(localized("Volume", m.language)) + " " +
                             (m.speaker_button ? std::to_string(m.volume) + "%" : localized("Unavailable", m.language));
  c.text(reading_inset(), top, fit(volume, layout_.hero_cols), s, kDim);
  c.text(reading_inset(), top + row, localized("Text size", m.language), s, kDim);
  for (auto control : {QuickControl::VolumeDown, QuickControl::Mute, QuickControl::VolumeUp,
                       QuickControl::TextNormal, QuickControl::TextLarge}) {
    const auto r = control_rect(control);
    const char* label = control == QuickControl::VolumeDown ? "-" : control == QuickControl::VolumeUp ? "+"
                        : control == QuickControl::Mute ? (m.muted ? "Unmute" : "Mute")
                        : control == QuickControl::TextNormal ? "Normal" : "Large";
    const bool audio = control == QuickControl::VolumeDown || control == QuickControl::VolumeUp || control == QuickControl::Mute;
    const bool selected = (control == QuickControl::Mute && m.muted) ||
                          (control == QuickControl::TextNormal && !m.large_text) ||
                          (control == QuickControl::TextLarge && m.large_text);
    c.fill_rect(r.x + s, r.y, r.w - 2 * s, r.h, selected ? kAccentDim : kButton);
    const auto text = fit(localized(label, m.language), cols_for(r.w - 4 * s, s));
    c.text(r.x + (r.w - Canvas::text_width(text, s)) / 2,
           r.y + (r.h - font::kGlyphHeight * s) / 2, text, s, audio && !m.speaker_button ? kFaint : kText);
  }
}

ReadingControl Ui::reading_hit(const UiModel& m, int x, int y) const {
  const int inset = reading_inset(), width = info_.width - 2 * inset;
  x -= ox_ + inset;
  y -= oy_;
  if (m.reading == Reading::None || x < 0 || x >= width ||
      y < info_.height - layout_.bottom_h || y >= info_.height)
    return ReadingControl::None;
  if (m.reading == Reading::Recall) return ReadingControl::Recall;
  if (x < width / 4) return ReadingControl::Previous;
  if (x >= width * 3 / 4) return ReadingControl::Next;
  return ReadingControl::TogglePause;
}

void Ui::draw_bottom(Canvas& c, const UiModel& m) {
  const int s = layout_.scale;
  const int y0 = info_.height - layout_.bottom_h;
  c.fill_rect(0, y0, info_.width, layout_.bottom_h, panel_.round ? bg_ : bar_);
  if (m.screen == Screen::Controls) {
    const auto r = control_rect(QuickControl::Close);
    c.fill_rect(r.x + s, r.y + s, r.w - 2 * s, r.h - 2 * s, kButton);
    const char* label = localized("Done", m.language);
    c.text(r.x + (r.w - Canvas::text_width(label, s)) / 2,
           r.y + (r.h - font::kGlyphHeight * s) / 2, label, s, kText);
    return;
  }
  if (m.reading != Reading::None) {
    const int inset = reading_inset(), width = info_.width - 2 * inset;
    auto button = [&](int left, int right, const char* label, uint16_t color) {
      c.fill_rect(left + s, y0 + s, right - left - 2 * s, layout_.bottom_h - 2 * s, kButton);
      const std::string text = fit(label, cols_for(right - left - 4 * s, s));
      c.text((left + right - Canvas::text_width(text, s)) / 2,
             y0 + (layout_.bottom_h - font::kGlyphHeight * s) / 2, text, s, color);
    };
    if (m.reading == Reading::Recall) {
      button(inset, inset + width, localized("Last reply", m.language), kText);
    } else {
      const int last = std::max(0, static_cast<int>(wrap_text(m.body, layout_.body_cols).size()) - body_rows(m));
      button(inset, inset + width / 4, "<", m.scroll > 0 ? kText : kFaint);
      button(inset + width / 4, inset + width * 3 / 4,
             localized(m.reading == Reading::Paused ? "Auto pages" : "Pause text", m.language), kAccent);
      button(inset + width * 3 / 4, inset + width, ">", m.scroll < last ? kText : kFaint);
    }
    return;
  }
  std::string hint = fit(m.hint, cols_for(info_.width - 4 * s - 2 * bottom_inset_, s));
  c.text((info_.width - Canvas::text_width(hint, s)) / 2, y0 + s, hint, s, kDim);
}

}  // namespace hg
