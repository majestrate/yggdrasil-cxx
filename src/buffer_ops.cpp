#include <yggdrasil/buffer_ops.hpp>

namespace yggdrasil {

std::string buffer_printer::str() const {
  std::string res;
  res.reserve(_buf.size() * 5);
  for (const auto &ch : _buf) {
    res += fmt::format("0x{:02x} ", int{ch});
  }
  return res;
}
} // namespace yggdrasil
