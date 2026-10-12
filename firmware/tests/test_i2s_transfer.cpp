#include <vector>
#include "check.hpp"
#include "i2s_transfer.hpp"

TEST("I2S: microphone timeout preserves the samples already read") {
  CHECK_EQ(hg::received_bytes(hg::TransferStatus::Timeout, 80), 80u);
  CHECK_EQ(hg::received_bytes(hg::TransferStatus::Error, 80), 0u);
}

TEST("I2S: partial timeout writes resume from the first unwritten byte") {
  struct Writer {
    std::vector<uint8_t> output;
    bool cancelled() { return false; }
    hg::TransferResult write(const uint8_t* p, size_t n) {
      const size_t take = output.empty() ? 2 : n;
      output.insert(output.end(), p, p + take);
      return {take == n ? hg::TransferStatus::Ok : hg::TransferStatus::Timeout, take};
    }
  } writer;
  const uint8_t pcm[] = {1, 2, 3, 4, 5, 6};
  CHECK_EQ(hg::write_pcm(pcm, sizeof(pcm), writer), sizeof(pcm));
  CHECK(writer.output == std::vector<uint8_t>({1, 2, 3, 4, 5, 6}));
}

TEST("I2S: cancellation and a stalled channel bound retries") {
  struct Writer {
    int calls = 0;
    bool cancel = false;
    bool cancelled() { return cancel; }
    hg::TransferResult write(const uint8_t*, size_t) { ++calls; return {hg::TransferStatus::Timeout, 0}; }
  } writer;
  const uint8_t pcm[] = {1, 2};
  CHECK_EQ(hg::write_pcm(pcm, sizeof(pcm), writer), 0u);
  CHECK_EQ(writer.calls, 3);
  writer.cancel = true;
  CHECK_EQ(hg::write_pcm(pcm, sizeof(pcm), writer), 0u);
  CHECK_EQ(writer.calls, 3);
}
