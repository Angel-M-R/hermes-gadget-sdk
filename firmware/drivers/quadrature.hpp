#pragma once
#include <cstdint>

namespace hg {
// Retry a congested app queue before consuming another encoder sample.
template <class Next, class Post>
void drain_encoder(int& pending, Next&& next, Post&& post) {
  for (;;) {
    if (pending) {
      if (!post(pending)) return;
      pending = 0;
    }
    if (!next(pending)) return;
  }
}

class Quadrature {
 public:
  void reset(uint8_t state) { state_ = state & 3; accumulated_ = 0; }
  int sample(uint8_t state) {
    static constexpr int8_t step[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    state &= 3;
    if ((state ^ state_) == 3) accumulated_ = 0;  // a missed edge cannot make a detent
    else accumulated_ += step[(state_ << 2) | state];
    state_ = state;
    if (accumulated_ >= 4 || accumulated_ <= -4) {
      const int direction = accumulated_ > 0 ? 1 : -1;
      accumulated_ = 0;
      return direction;
    }
    return 0;
  }
 private:
  uint8_t state_ = 0;
  int accumulated_ = 0;
};
}  // namespace hg
