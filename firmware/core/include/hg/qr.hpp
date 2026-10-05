// QR codes (ISO/IEC 18004) for short texts such as a Wi-Fi network's join
// code: byte mode, error correction level M, versions 1 to 6, so up to 106
// bytes. Portable and deterministic, like the rest of the UI.
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace hg {

class QrCode {
 public:
  // Encodes `text` with the given mask (0..7), or with the mask that scores
  // best when `mask` is -1. Returns an empty code (size() 0) when the text is
  // longer than a version 6 code holds.
  static QrCode encode(std::string_view text, int mask = -1);

  // Modules per side, without the quiet zone; 0 for an empty code.
  int size() const { return size_; }
  int version() const { return version_; }
  int mask() const { return mask_; }
  bool dark(int x, int y) const { return dark_[static_cast<size_t>(y * size_ + x)] != 0; }

 private:
  int size_ = 0, version_ = 0, mask_ = 0;
  std::vector<uint8_t> dark_;
};

}  // namespace hg
