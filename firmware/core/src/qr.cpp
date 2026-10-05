#include "hg/qr.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <utility>

namespace hg {
namespace {

constexpr int kMaxVersion = 6;
// Error correction level M: codewords per block and blocks, for versions 1..6.
constexpr int kEcPerBlock[kMaxVersion + 1] = {0, 10, 16, 26, 18, 24, 16};
constexpr int kBlocks[kMaxVersion + 1] = {0, 1, 1, 1, 2, 2, 4};
constexpr int kLevelM = 0;  // level M's two bits in the format information

// Modules left for codewords once the finder, timing and alignment patterns
// and the format information are placed (versions below 7 carry no version
// information).
int codeword_modules(int version) {
  int n = (16 * version + 128) * version + 64;
  if (version >= 2) {
    const int align = version / 7 + 2;
    n -= (25 * align - 10) * align - 55;
  }
  return n;
}

int data_codewords(int version) {
  return codeword_modules(version) / 8 - kEcPerBlock[version] * kBlocks[version];
}

// GF(256) over x^8 + x^4 + x^3 + x^2 + 1, the field of QR's Reed-Solomon codes.
struct Field {
  uint8_t exp[510];
  uint8_t log[256] = {};
  Field() {
    int x = 1;
    for (int i = 0; i < 255; ++i) {
      exp[i] = exp[i + 255] = static_cast<uint8_t>(x);
      log[x] = static_cast<uint8_t>(i);
      x <<= 1;
      if (x & 0x100) x ^= 0x11D;
    }
  }
  uint8_t mul(uint8_t a, uint8_t b) const { return a && b ? exp[log[a] + log[b]] : 0; }
};

const Field& field() {
  static const Field f;
  return f;
}

// `count` error correction codewords for `data`: the remainder of
// data(x) * x^count divided by (x - a^0)(x - a^1)...(x - a^(count - 1)).
std::vector<uint8_t> error_correction(const std::vector<uint8_t>& data, int count) {
  const Field& f = field();
  const size_t n = static_cast<size_t>(count);
  std::vector<uint8_t> gen(1, 1);  // coefficients, highest power first
  for (size_t i = 0; i < n; ++i) {
    std::vector<uint8_t> next(gen.size() + 1, 0);
    for (size_t j = 0; j < gen.size(); ++j) {
      next[j] ^= gen[j];
      next[j + 1] ^= f.mul(gen[j], f.exp[i]);
    }
    gen.swap(next);
  }
  std::vector<uint8_t> rem(n, 0);
  for (uint8_t byte : data) {
    const uint8_t factor = byte ^ rem[0];
    for (size_t i = 0; i + 1 < n; ++i) rem[i] = rem[i + 1] ^ f.mul(gen[i + 1], factor);
    rem[n - 1] = f.mul(gen[n], factor);
  }
  return rem;
}

// Bits appended most significant first.
struct Bits {
  std::vector<uint8_t> bytes;
  int count = 0;
  void put(unsigned value, int n) {
    for (int i = n - 1; i >= 0; --i, ++count) {
      if (count % 8 == 0) bytes.push_back(0);
      if ((value >> i) & 1) bytes.back() = static_cast<uint8_t>(bytes.back() | (0x80 >> (count % 8)));
    }
  }
};

struct Grid {
  int size;
  std::vector<uint8_t> dark, fixed;  // fixed: part of a pattern, not of the data
  explicit Grid(int version)
      : size(17 + 4 * version), dark(static_cast<size_t>(size * size)), fixed(static_cast<size_t>(size * size)) {}
  size_t at(int x, int y) const { return static_cast<size_t>(y * size + x); }
  void set(int x, int y, bool on) {
    dark[at(x, y)] = on;
    fixed[at(x, y)] = 1;
  }
};

// Format information: level M, the mask and their BCH check bits, twice.
void place_format(Grid& g, int mask) {
  const int data = kLevelM << 3 | mask;
  int rem = data;
  for (int i = 0; i < 10; ++i) rem = (rem << 1) ^ ((rem >> 9) * 0x537);
  const int bits = (data << 10 | rem) ^ 0x5412;
  auto bit = [bits](int i) { return ((bits >> i) & 1) != 0; };
  const int n = g.size;
  for (int i = 0; i <= 5; ++i) g.set(8, i, bit(i));
  g.set(8, 7, bit(6));
  g.set(8, 8, bit(7));
  g.set(7, 8, bit(8));
  for (int i = 9; i < 15; ++i) g.set(14 - i, 8, bit(i));
  for (int i = 0; i < 8; ++i) g.set(n - 1 - i, 8, bit(i));
  for (int i = 8; i < 15; ++i) g.set(8, n - 15 + i, bit(i));
  g.set(8, n - 8, true);  // always dark
}

void place_patterns(Grid& g, int version) {
  const int n = g.size;
  for (int i = 0; i < n; ++i) {
    g.set(6, i, i % 2 == 0);
    g.set(i, 6, i % 2 == 0);
  }
  // Finder patterns with their light separators, at three corners.
  for (auto [cx, cy] : {std::pair<int, int>{3, 3}, {n - 4, 3}, {3, n - 4}}) {
    for (int dy = -4; dy <= 4; ++dy) {
      for (int dx = -4; dx <= 4; ++dx) {
        const int x = cx + dx, y = cy + dy;
        if (x < 0 || y < 0 || x >= n || y >= n) continue;
        const int ring = std::max(std::abs(dx), std::abs(dy));
        g.set(x, y, ring != 2 && ring != 4);
      }
    }
  }
  // Versions 2..6 have a single alignment pattern, towards the bottom right.
  if (version >= 2) {
    for (int dy = -2; dy <= 2; ++dy) {
      for (int dx = -2; dx <= 2; ++dx) g.set(n - 7 + dx, n - 7 + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
    }
  }
  place_format(g, 0);  // reserves the area; the chosen mask's bits replace it
}

// Codewords go up and down two-module columns from the bottom right, around
// the patterns. Remainder modules stay light.
void place_data(Grid& g, const std::vector<uint8_t>& codewords) {
  const int n = g.size;
  const size_t total = codewords.size() * 8;
  size_t bit = 0;
  for (int right = n - 1; right >= 1; right -= 2) {
    if (right == 6) right = 5;  // the vertical timing pattern
    const bool upward = ((right + 1) & 2) == 0;
    for (int step = 0; step < n; ++step) {
      const int y = upward ? n - 1 - step : step;
      for (int x = right; x >= right - 1; --x) {
        if (g.fixed[g.at(x, y)]) continue;
        g.dark[g.at(x, y)] = bit < total && ((codewords[bit / 8] >> (7 - bit % 8)) & 1);
        ++bit;
      }
    }
  }
}

bool masked(int mask, int x, int y) {
  switch (mask) {
    case 0: return (x + y) % 2 == 0;
    case 1: return y % 2 == 0;
    case 2: return x % 3 == 0;
    case 3: return (x + y) % 3 == 0;
    case 4: return (x / 3 + y / 2) % 2 == 0;
    case 5: return x * y % 2 + x * y % 3 == 0;
    case 6: return (x * y % 2 + x * y % 3) % 2 == 0;
    default: return ((x + y) % 2 + x * y % 3) % 2 == 0;
  }
}

void apply_mask(Grid& g, int mask) {
  for (int y = 0; y < g.size; ++y) {
    for (int x = 0; x < g.size; ++x) {
      if (!g.fixed[g.at(x, y)] && masked(mask, x, y)) g.dark[g.at(x, y)] ^= 1;
    }
  }
}

// The standard's penalty score: long runs, 2x2 blocks, finder look-alikes and
// an uneven share of dark modules each make a symbol harder to read.
int penalty(const Grid& g) {
  static constexpr uint8_t kFinderLeft[11] = {1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0};
  static constexpr uint8_t kFinderRight[11] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1};
  const int n = g.size;
  int score = 0;
  std::vector<uint8_t> line(static_cast<size_t>(n + 8));  // four light modules either side
  for (int pass = 0; pass < 2; ++pass) {
    for (int a = 0; a < n; ++a) {
      for (int b = 0; b < n; ++b) line[static_cast<size_t>(b + 4)] = g.dark[pass ? g.at(a, b) : g.at(b, a)];
      int run = 1;
      for (int b = 1; b < n; ++b) {
        if (line[static_cast<size_t>(b + 4)] != line[static_cast<size_t>(b + 3)]) {
          run = 1;
        } else if (++run == 5) {
          score += 3;
        } else if (run > 5) {
          ++score;
        }
      }
      for (size_t k = 0; k + 11 <= line.size(); ++k) {
        if (std::equal(kFinderLeft, kFinderLeft + 11, line.begin() + static_cast<std::ptrdiff_t>(k)) ||
            std::equal(kFinderRight, kFinderRight + 11, line.begin() + static_cast<std::ptrdiff_t>(k)))
          score += 40;
      }
    }
  }
  int dark = 0;
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      dark += g.dark[g.at(x, y)];
      if (x + 1 < n && y + 1 < n) {
        const uint8_t c = g.dark[g.at(x, y)];
        if (g.dark[g.at(x + 1, y)] == c && g.dark[g.at(x, y + 1)] == c && g.dark[g.at(x + 1, y + 1)] == c) score += 3;
      }
    }
  }
  const int total = n * n;
  score += 10 * (std::abs(dark * 20 - total * 10) / total);
  return score;
}

}  // namespace

QrCode QrCode::encode(std::string_view text, int mask) {
  QrCode qr;
  int version = 0;
  for (int v = 1; v <= kMaxVersion && !version; ++v) {
    if (12 + 8 * text.size() <= static_cast<size_t>(data_codewords(v)) * 8) version = v;
  }
  if (!version || mask < -1 || mask > 7) return qr;

  // Byte mode: mode, length, the bytes, a terminator, then the standard padding.
  const int capacity = data_codewords(version) * 8;
  Bits bits;
  bits.put(0x4, 4);
  bits.put(static_cast<unsigned>(text.size()), 8);
  for (char c : text) bits.put(static_cast<uint8_t>(c), 8);
  bits.put(0, std::min(4, capacity - bits.count));
  bits.put(0, (8 - bits.count % 8) % 8);
  for (unsigned pad = 0xEC; bits.count < capacity; pad ^= 0xEC ^ 0x11) bits.put(pad, 8);

  // Split into blocks, add each block's error correction, then interleave.
  const int blocks = kBlocks[version], ec = kEcPerBlock[version];
  const int raw = codeword_modules(version) / 8;
  const int short_blocks = blocks - raw % blocks;
  const int short_data = raw / blocks - ec;
  std::vector<std::vector<uint8_t>> data(static_cast<size_t>(blocks)), checks;
  auto next = bits.bytes.begin();
  for (int b = 0; b < blocks; ++b) {
    const std::ptrdiff_t len = short_data + (b < short_blocks ? 0 : 1);
    data[static_cast<size_t>(b)].assign(next, next + len);
    next += len;
    checks.push_back(error_correction(data[static_cast<size_t>(b)], ec));
  }
  std::vector<uint8_t> codewords;
  for (size_t i = 0; i <= static_cast<size_t>(short_data); ++i) {
    for (const auto& block : data) {
      if (i < block.size()) codewords.push_back(block[i]);
    }
  }
  for (size_t i = 0; i < static_cast<size_t>(ec); ++i) {
    for (const auto& check : checks) codewords.push_back(check[i]);
  }

  Grid base(version);
  place_patterns(base, version);
  place_data(base, codewords);
  auto with_mask = [&base](int m) {
    Grid g = base;
    apply_mask(g, m);
    place_format(g, m);
    return g;
  };
  if (mask < 0) {
    int best = INT_MAX;
    for (int m = 0; m < 8; ++m) {
      const int score = penalty(with_mask(m));
      if (score < best) {
        best = score;
        mask = m;
      }
    }
  }
  Grid g = with_mask(mask);
  qr.size_ = g.size;
  qr.version_ = version;
  qr.mask_ = mask;
  qr.dark_ = std::move(g.dark);
  return qr;
}

}  // namespace hg
