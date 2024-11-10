#pragma once
#include "format.hpp"
#include "murmur3.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>
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

  std::string to_string() const { return _bits.to_string(); };

  /// merge
  BloomFilter<M, K> &operator|=(const BloomFilter<M, K, Hasher_t> &other) {
    _bits |= other._bits;
    return *this;
  }

  constexpr bool operator==(const BloomFilter<M, K, Hasher_t> &other) const {
    return _bits == other._bits;
  };

  /// returns how many bytes it takes to encode this bloom filter
  static constexpr size_t byte_size() { return (M / 8) + (M % 8 ? 1 : 0); }

  void merge(const BloomFilter<M, K, Hasher_t> &other) { *this |= other; }

  /// serialize to bytes
  template <typename Iter_t>
  constexpr Iter_t encode(Iter_t begin, Iter_t end) const {
    using val_t = typename std::iterator_traits<Iter_t>::value_type;
    constexpr size_t bytes_per_iteration = sizeof(val_t);
    constexpr size_t bits_per_iteration = 8 * bytes_per_iteration;
    auto dist = bytes_per_iteration * std::distance(begin, end);
    if (dist < byte_size())
      throw std::range_error{fmt::format(
          "encode() iterator range too small: {} < {}", dist, byte_size())};
    auto itr = begin;
    for (size_t idx = 0; idx < M; ++idx) {
      if (idx % bits_per_iteration == 0)
        *(itr) = val_t{0}; // zero out the value first
      if (_bits.test(idx))
        (*itr) |= val_t{1} << (idx % bits_per_iteration);
      if ((1 + idx) % bits_per_iteration == 0)
        ++itr;
    }
    return itr;
  }

  /// deserialize from bytes
  template <typename Iter_t> constexpr Iter_t decode(Iter_t begin, Iter_t end) {
    using val_t = typename std::iterator_traits<Iter_t>::value_type;
    constexpr size_t bytes_per_iteration = sizeof(val_t);
    constexpr size_t bits_per_iteration = 8 * bytes_per_iteration;
    auto dist = bytes_per_iteration * std::distance(begin, end);
    if (dist < byte_size())
      throw std::range_error{fmt::format(
          "decode() iterator range too small: {} < {}", dist, byte_size())};
    auto itr = begin;
    for (size_t idx = 0; idx < M; ++idx) {
      if ((*itr) & (val_t{1} << (idx % bits_per_iteration)))
        _bits.set(idx);
      if ((1 + idx) % bits_per_iteration == 0)
        ++itr;
    }
    return itr;
  }
};
} // namespace yggdrasil
