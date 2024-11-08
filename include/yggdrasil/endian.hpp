#pragma once

#include <endian.h>

namespace yggdrasil {

inline uint16_t native16_from_big(const void *ptr) {
  uint16_t x = *reinterpret_cast<const uint16_t *>(ptr);
  return be16toh(x);
}
} // namespace yggdrasil
