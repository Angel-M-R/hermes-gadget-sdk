#pragma once
#include <cstddef>
#include <cstdint>

namespace hg {
enum class TransferStatus { Ok, Timeout, Error };
inline size_t received_bytes(TransferStatus status, size_t bytes) {
  return status == TransferStatus::Error ? 0 : bytes;
}
struct TransferResult { TransferStatus status; size_t bytes; };

// Each write has a driver timeout. Three attempts with no progress bound a
// broken channel; cancellation discards only the tail of this chunk.
template <class Writer>
size_t write_pcm(const uint8_t* data, size_t size, Writer& writer) {
  size_t offset = 0;
  unsigned stalled = 0;
  while (offset < size && !writer.cancelled()) {
    const TransferResult result = writer.write(data + offset, size - offset);
    if (result.bytes > size - offset) break;
    offset += result.bytes;
    stalled = result.bytes ? 0 : stalled + 1;
    if (result.status == TransferStatus::Error || stalled >= 3) break;
  }
  return offset;
}
}  // namespace hg
