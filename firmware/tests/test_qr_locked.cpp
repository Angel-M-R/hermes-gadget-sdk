#include <cstring>
#include <string>
#include <vector>
#include "../core/third_party/qrcode/qrcode.h"

int main() {
  std::vector<uint8_t> modules(qrcode_getBufferSize(3), 0xa5);
  const auto before = modules;
  QRCode code{};
  uint8_t text[] = {'A'};
  if (qrcode_initBytes(&code, modules.data(), 1, ECC_LOW, text, 1) != -1 || modules != before) return 1;
  if (qrcode_initBytes(&code, modules.data(), 3, 16, text, 1) != -1 || modules != before) return 1;
  if (qrcode_initBytes(&code, modules.data(), 3, ECC_LOW, text, 1) != 0 || code.size != 29) return 1;
  const std::string huge(65536, 'A');
  return qrcode_initText(&code, modules.data(), 3, ECC_LOW, huge.c_str()) == -1 ? 0 : 1;
}
