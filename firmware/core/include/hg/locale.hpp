#pragma once

namespace hg {
enum class Language { English, Spanish };
// Translate device-owned labels only; agent messages are displayed unchanged.
const char* localized(const char* english, Language language);
}  // namespace hg
