#pragma once
#include "bloom_filter.hpp"
#include <array>
#include <cstdint>

namespace yggdrasil {
class Bloom {
  static constexpr auto bloomFilterF = uint64_t{16};
  static constexpr auto bloomFilterU = bloomFilterF * 8;
  static constexpr auto bloomFilterB = bloomFilterU * 8;
  static constexpr auto bloomFilterM = bloomFilterB * 8;
  static constexpr auto bloomFilterK = uint64_t{8};

  BloomFilter<bloomFilterM, bloomFilterK> _filter{};

public:
  void add_key(const std::array<uint8_t, 32> &pubkey);

  bool has_key(const std::array<uint8_t, 32> &pubkey) const;
};
} // namespace yggdrasil
