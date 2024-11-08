#include <yggdrasil/bloom.hpp>

namespace yggdrasil {
void Bloom::add_key(const std::array<uint8_t, 32> &pubkey) {
  _filter.add(pubkey.begin(), pubkey.end());
}
bool Bloom::has_key(const std::array<uint8_t, 32> &pubkey) const {
  return _filter.test(pubkey.begin(), pubkey.end());
}
} // namespace yggdrasil
