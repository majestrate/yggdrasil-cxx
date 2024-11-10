#pragma once

#include <cstdint>
#include <endian.h>

namespace yggdrasil {

inline uint16_t native16_from_big(const void *ptr) {
  uint16_t x = *reinterpret_cast<const uint16_t *>(ptr);
  return be16toh(x);
}

inline uint64_t native64_from_big(const void *ptr) {
  uint64_t x = *reinterpret_cast<const uint64_t *>(ptr);
  return be64toh(x);
}
} // namespace yggdrasil
