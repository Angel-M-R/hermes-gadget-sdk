#include "check.hpp"
#include "quadrature.hpp"
#include <vector>

TEST("encoder: queued edges preserve multiple detents between touch polls") {
  hg::Quadrature encoder;
  int detents = 0;
  for (int i = 0; i < 5; ++i) for (uint8_t state : {uint8_t(2), uint8_t(3), uint8_t(1), uint8_t(0)})
    detents += encoder.sample(state);
  CHECK_EQ(detents, 5);
  int reverse = 0;
  for (uint8_t state : {uint8_t(1), uint8_t(3), uint8_t(2), uint8_t(0)}) reverse += encoder.sample(state);
  CHECK_EQ(reverse, -1);
}

TEST("encoder: bounce and missed edges do not create a false detent") {
  hg::Quadrature encoder;
  for (uint8_t state : {uint8_t(2), uint8_t(0), uint8_t(2), uint8_t(0), uint8_t(3), uint8_t(0)})
    CHECK_EQ(encoder.sample(state), 0);
}

TEST("encoder: a missed edge discards an accumulated partial detent") {
  hg::Quadrature encoder;
  // 0->2 leaves +1, then 2->1 misses an edge. The next three +1
  // transitions cannot complete the old detent.
  for (uint8_t state : {uint8_t(2), uint8_t(1), uint8_t(0), uint8_t(2), uint8_t(3)})
    CHECK_EQ(encoder.sample(state), 0);
  CHECK_EQ(encoder.sample(1), 1);
}

TEST("encoder: congested delivery retries every detent in order") {
  const std::vector<int> samples{0, 1, 0, 1, -1, 0, -1};
  std::vector<int> delivered;
  size_t consumed = 0;
  int pending = 0, capacity = 1;
  auto next = [&](int& direction) {
    if (consumed == samples.size()) return false;
    direction = samples[consumed++];
    return true;
  };
  auto post = [&](int direction) {
    if (!capacity) return false;
    --capacity;
    delivered.push_back(direction);
    return true;
  };
  hg::drain_encoder(pending, next, post);
  CHECK_EQ(delivered.size(), 1u);
  CHECK_EQ(pending, 1);
  CHECK_EQ(consumed, 4u);
  hg::drain_encoder(pending, next, post);  // still full: consume nothing
  CHECK_EQ(consumed, 4u);
  capacity = 3;
  hg::drain_encoder(pending, next, post);
  CHECK(delivered == std::vector<int>({1, 1, -1, -1}));
  CHECK_EQ(pending, 0);
  CHECK_EQ(consumed, samples.size());
}
