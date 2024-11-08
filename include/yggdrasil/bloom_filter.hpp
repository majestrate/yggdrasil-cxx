#pragma once
#include "murmur3.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

namespace yggdrasil {
template <uint64_t M, uint64_t K, typename Hasher_t = murmur3::Digest>
class BloomFilter {
  std::bitset<M> _bits{};

  static constexpr uint64_t location(const std::array<uint64_t, 4> &h,
                                     uint64_t ii) {
    return (h[ii % 2] + ii * h[2 + (((ii + (ii % 2)) % 4) / 2)]) % M;
  }

public:
  /// add data to the bloom filter unconditionally.
  template <typename Iter_t> void add(Iter_t begin, Iter_t end) {
    const auto h = Hasher_t::hash256(begin, end);
    for (uint64_t i = 0; i < K; ++i)
      _bits.set(location(h, i));
  }

  /// returns true if the data is probably in the bloom filter.
  template <typename Iter_t>
  constexpr bool test(Iter_t begin, Iter_t end) const {
    const auto h = Hasher_t::hash256(begin, end);
    for (uint64_t i = 0; i < K; ++i) {
      if (!_bits.test(location(h, i)))
        return false;
    }
    return true;
  }

  /// adds the data to the bloom filter
  /// returns true if the data was already probably present in the filter.
  /// returns false if the data was definately not in the bloom filter.
  template <typename Iter_t> bool test_or_add(Iter_t begin, Iter_t end) {
    bool present = true;
    const auto h = Hasher_t::hash256(begin, end);
    for (uint64_t i = 0; i < K; ++i) {
      const auto l = location(h, i);
      present = present and _bits.test(l);
      _bits.set(l);
    }
    return present;
  }

  /// merge
  BloomFilter<M, K> &operator|=(const BloomFilter<M, K, Hasher_t> &other) {
    _bits |= other._bits;
    return *this;
  }

  void merge(const BloomFilter<M, K, Hasher_t> &other) { *this |= other; }

  /// serialize to bytes
  std::vector<uint8_t> to_bytes() const {
    std::vector<uint8_t> ret;
    ret.resize((M / 8) + (M % 8 ? 1 : 0), 0);

    for (size_t idx = 0; idx < M; ++idx) {
      if (_bits.test(idx))
        ret[idx / 8] |= 1 << (idx % 8);
    }
    return ret;
  }
};
} // namespace yggdrasil
