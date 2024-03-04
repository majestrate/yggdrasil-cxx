#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#include <fmt/format.h>

namespace yggdrasil {

namespace {
/// max size of a varuint in bytes
constexpr size_t max_varuint_64bit_bytes = 10;
} // namespace
/// reads a golang varint in buffer, returns decode value and where we left off.
template <typename Iter_t>
std::pair<uint64_t, Iter_t> read_golang_varuint(Iter_t begin, Iter_t end) {
  uint64_t x{};
  uint32_t s{};
  for (Iter_t itr = begin; itr != end; itr++) {
    const std::uint8_t b = *itr;
    if (b < 0x80) {
      if (std::distance(begin, itr) == max_varuint_64bit_bytes - 1 and b > 1)
        throw std::invalid_argument{"decode varint underflow"};
      return std::make_pair(uint64_t{x | b << s}, itr);
    }
    x |= (b & 0x7f) << s;
    s += 7;
  }
  throw std::invalid_argument{"decode varint 64bit overflow"};
}

/// writes a golang varint
template <typename Iter_t>
Iter_t write_golang_varuint(uint64_t x, Iter_t begin, Iter_t end) {
  constexpr uint64_t small = 0x000000000080UL;
  constexpr uint64_t mask = 0x0000000000FFUL;

  auto itr = begin;

  while (x >= small) {
    if (itr >= end)
      return itr;

    *itr = (x & mask) | small;
    x >>= 7;
    ++itr;
  }
  *itr = (x & mask) | small;
  return itr;
}

} // namespace yggdrasil
