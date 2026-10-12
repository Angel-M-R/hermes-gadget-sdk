#include "hg/text.hpp"

namespace hg {
uint32_t next_character(std::string_view text, size_t& offset) {
  if (offset >= text.size()) return 0;
  const auto first = static_cast<uint8_t>(text[offset++]);
  if (first < 0x80) return first;
  const int bytes = first >= 0xC2 && first <= 0xDF ? 2 : first >= 0xE0 && first <= 0xEF ? 3
                    : first >= 0xF0 && first <= 0xF4 ? 4 : 0;
  if (!bytes || offset + bytes - 1 > text.size()) return '?';
  uint32_t cp = first & (0x7F >> bytes);
  for (int i = 0; i < bytes - 1; ++i) {
    const auto b = static_cast<uint8_t>(text[offset + i]);
    if ((b & 0xC0) != 0x80) return '?';
    cp = (cp << 6) | (b & 0x3F);
  }
  if ((bytes == 2 && cp < 0x80) || (bytes == 3 && cp < 0x800) || (bytes == 4 && cp < 0x10000) ||
      (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) return '?';
  offset += bytes - 1;
  return cp;
}

size_t text_length(std::string_view text) {
  size_t offset = 0, count = 0;
  while (offset < text.size()) { next_character(text, offset); ++count; }
  return count;
}

std::string_view text_prefix(std::string_view text, size_t characters) {
  size_t offset = 0;
  while (characters-- && offset < text.size()) next_character(text, offset);
  return text.substr(0, offset);
}
}  // namespace hg
