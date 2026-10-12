#pragma once
#include <utility>

namespace hg {
template <class Cleanup>
class ScopeExit {
 public:
  explicit ScopeExit(Cleanup cleanup) : cleanup_(std::move(cleanup)) {}
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ~ScopeExit() { if (active_) cleanup_(); }
  void dismiss() { active_ = false; }

 private:
  Cleanup cleanup_;
  bool active_ = true;
};
}  // namespace hg
