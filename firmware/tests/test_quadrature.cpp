#include "check.hpp"
#include "quadrature.hpp"

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
