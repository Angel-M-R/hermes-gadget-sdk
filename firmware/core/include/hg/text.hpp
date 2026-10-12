// UTF-8 character boundaries for the device's fixed-width bitmap font.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hg {
// Invalid input consumes one byte and yields '?', so callers always progress.
uint32_t next_character(std::string_view text, size_t& offset);
size_t text_length(std::string_view text);
std::string_view text_prefix(std::string_view text, size_t characters);
}  // namespace hg
