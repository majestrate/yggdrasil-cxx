#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#include <fmt/format.h>

namespace yggdrasil {

/// max size of a varuint in bytes
constexpr size_t max_varuint_64bit_bytes = 10;

/// reads a golang varint in buffer, returns decode value and where we left off.
template <typename Iter_t>
std::tuple<uint64_t, Iter_t> read_golang_varuint(Iter_t begin, Iter_t end) {
  uint64_t x{};
  uint32_t s{};
  for (Iter_t itr = begin; itr != end; itr++) {
    const std::uint8_t b = *itr;
    if (b < 0x80) {
      if (std::distance(begin, itr) == max_varuint_64bit_bytes - 1 and b > 1)
        throw std::invalid_argument{
            fmt::format("decode varint underflow: '{}' at '{}'", b, itr)};
      return x | b << s;
    }
    x |= (b & 0x7f) << s;
    s += 7;
  }
  throw std::invalid_argument{"decode varint 64bit overflow"};
}
} // namespace yggdrasil
