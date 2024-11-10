#pragma once
#include "bloom_filter.hpp"
#include "endian.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <stdexcept>

namespace yggdrasil {
class Bloom {
  static constexpr auto bloomFilterF = uint64_t{16};
  static constexpr auto bloomFilterU = bloomFilterF * 8;
  static constexpr auto bloomFilterB = bloomFilterU * 8;
  static constexpr auto bloomFilterM = bloomFilterB * 8;
  static constexpr auto bloomFilterK = uint64_t{8};

  BloomFilter<bloomFilterM, bloomFilterK> _filter{};

  using filter_buf_t = std::array<uint64_t, bloomFilterU>;

public:
  static constexpr uint64_t byte_size() { return bloomFilterB; };
  static constexpr uint64_t u64_size() { return bloomFilterU; };

  void add_key(const std::array<uint8_t, 32> &pubkey);

  bool has_key(const std::array<uint8_t, 32> &pubkey) const;

  /// converts native endian bitset to big endian output
  template <typename Iter_t>
  constexpr Iter_t encode(Iter_t begin, Iter_t end) const {
    using val_t = typename std::iterator_traits<Iter_t>::value_type;
    constexpr size_t bytes_per_iteration = sizeof(val_t);
    auto dist = bytes_per_iteration * std::distance(begin, end);
    if (dist < byte_size())
      throw std::range_error{fmt::format(
          "encode() iterator range too small: {} < {}", dist, byte_size())};
    filter_buf_t tmp;
    _filter.encode(tmp.begin(), tmp.end());
    // make it big endian
    std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                   [](filter_buf_t::value_type x) { return htobe64(x); });
    // copy it out
    return std::copy(reinterpret_cast<const val_t *>(tmp.begin()),
                     reinterpret_cast<const val_t *>(tmp.end()), begin);
  }

  /// takes in big endian input and converts it to native endian bitsett
  template <typename Iter_t> constexpr Iter_t decode(Iter_t begin, Iter_t end) {
    using val_t = typename std::iterator_traits<Iter_t>::value_type;
    constexpr size_t bytes_per_iteration = sizeof(val_t);
    constexpr size_t num_iterations =
        sizeof(filter_buf_t::value_type) / bytes_per_iteration;
    using iteration_buf_t = std::array<val_t, num_iterations>;
    auto dist = bytes_per_iteration * std::distance(begin, end);
    if (dist < byte_size())
      throw std::range_error{fmt::format(
          "decode() iterator range too small: {} < {}", dist, byte_size())};
    filter_buf_t tmp;
    auto itr = begin;
    for (auto &x : tmp) {
      iteration_buf_t buf;
      std::copy_n(itr, num_iterations, buf.begin());
      x = native64_from_big(buf.data());
      itr += num_iterations;
    }

    _filter.decode(tmp.begin(), tmp.end());
    return itr;
  }

  constexpr bool operator==(const Bloom &other) const {
    return _filter == other._filter;
  };
  std::string to_string() const { return _filter.to_string(); };
};

template <typename Pubkey_t> class BloomState {
  std::pmr::vector<Bloom> _send_blooms;
  std::pmr::vector<Bloom> _recv_blooms;
  std::pmr::vector<Pubkey_t> _pubkeys;
  std::vector<bool> _ontree;
  std::vector<bool> _zdirty;

public:
  using PubkeyAlloc_t = std::pmr::polymorphic_allocator<Pubkey_t>;
  using BloomAlloc_t = std::pmr::polymorphic_allocator<Bloom>;
  explicit BloomState(PubkeyAlloc_t &pk_alloc, BloomAlloc_t &b_alloc)
      : _send_blooms{b_alloc}, _recv_blooms{b_alloc}, _pubkeys{pk_alloc} {}

  /// iterate over each entry in the state (mutable).
  template <typename Visit_t> void for_each(Visit_t &&visitor) {
    for (size_t idx = 0; idx < _pubkeys.size(); ++idx) {
      visitor(_pubkeys[idx], _send_blooms[idx], _recv_blooms[idx], _ontree[idx],
              _zdirty[idx]);
    }
  }

  void add_state(Pubkey_t pk, Bloom send, Bloom recv, bool ontree,
                 bool zdirty) {
    size_t idx{};
    std::optional<size_t> last_empty = std::nullopt;
    static const Pubkey_t empty{};
    for (const auto &x : _pubkeys) {
      if (x == pk) {
        last_empty = idx;
        break;
      }

      if (x == empty)
        last_empty = idx;

      ++idx;
    }
    if (not last_empty) {
      idx = _pubkeys.size();
      _send_blooms.emplace_back();
      _recv_blooms.emplace_back();
      _pubkeys.emplace_back();
      _ontree.emplace_back();
      _zdirty.emplace_back();
    } else {
      idx = *last_empty;
    }
    _send_blooms[idx] = std::move(send);
    _recv_blooms[idx] = std::move(recv);
    _pubkeys[idx] = std::move(pk);
    _ontree[idx] = ontree;
    _zdirty[idx] = zdirty;
  }
};
} // namespace yggdrasil
