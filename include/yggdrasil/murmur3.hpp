#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <unistd.h>

namespace murmur3 {

class Digest {
  static constexpr uint64_t c1_128 = 0x87c37b91114253d5;
  static constexpr uint64_t c2_128 = 0x4cf5ad432745937f;
  static constexpr size_t block_size = 16;

  static inline void rotl(uint64_t &_x, int8_t r) {
    uint64_t x{_x};
    _x = (x << r) | (x >> (64 - r));
  }

  static inline void unpack_block(const uint8_t *ptr, uint64_t &_k1,
                                  uint64_t &_k2) {
    uint64_t k1, k2;
    memcpy(&k1, ptr, 8);
    memcpy(&k2, ptr + 8, 8);
    _k1 = htole64(k1);
    _k2 = htole64(k2);
  }

  uint64_t _h1, _h2;

  /// mix 16 byte blocks
  void bmix(const uint8_t *ptr, size_t nblocks) {
    for (size_t i = 0; i < nblocks; ++i) {
      uint64_t k1, k2;
      unpack_block(ptr, k1, k2);
      bmix_words(k1, k2);
      ptr += block_size;
    }
  }

  inline void bmix_words(uint64_t k1, uint64_t k2) {
    k1 *= c1_128;
    rotl(k1, 31);
    k1 *= c2_128;
    _h1 ^= k1;

    rotl(_h1, 27);
    _h1 += _h2;
    _h1 = _h1 * 5 + 0x52dce729;

    k2 *= c2_128;
    rotl(k2, 33);
    k2 *= c1_128;
    _h2 ^= k2;

    rotl(_h2, 31);
    _h2 += _h1;
    _h2 = _h2 * 5 + 0x38495ab5;
  }

  static inline void fmix64(uint64_t &_k) {
    uint64_t k{_k};
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    _k = k;
  }

  template <bool pad_tail>
  void sum128(size_t length, const uint8_t *tail_ptr, size_t tail_len,
              uint64_t &_out1, uint64_t &_out2) {
    uint64_t h1{_h1};
    uint64_t h2{_h2};
    uint64_t k1{}, k2{};
    if (pad_tail) {
      switch ((tail_len + 1) & 15) {
      case 15:
        k2 ^= uint64_t{1} << 48;
        break;
      case 14:
        k2 ^= uint64_t{1} << 40;
        break;
      case 13:
        k2 ^= uint64_t{1} << 32;
        break;
      case 12:
        k2 ^= uint64_t{1} << 24;
        break;
      case 11:
        k2 ^= uint64_t{1} << 16;
        break;
      case 10:
        k2 ^= uint64_t{1} << 8;
        break;
      case 9:
        k2 ^= uint64_t{1};
        k2 *= c2_128;
        rotl(k2, 33);
        k2 *= c1_128;
        h2 ^= k2;
        break;
      case 8:
        k1 ^= uint64_t{1} << 56;
        break;
      case 7:
        k1 ^= uint64_t{1} << 48;
        break;
      case 6:
        k1 ^= uint64_t{1} << 40;
        break;
      case 5:
        k1 ^= uint64_t{1} << 32;
        break;
      case 4:
        k1 ^= uint64_t{1} << 24;
        break;
      case 3:
        k1 ^= uint64_t{1} << 16;
        break;
      case 2:
        k1 ^= uint64_t{1} << 8;
        break;
      case 1:
        k1 ^= uint64_t{1};
        k1 *= c1_128;
        rotl(k1, 31);
        k1 *= c2_128;
        h1 ^= k1;
        break;
      }
    }
    switch (tail_len & 15) {
    case 15:
      k2 ^= uint64_t{tail_ptr[14]} << 48;
    case 14:
      k2 ^= uint64_t{tail_ptr[13]} << 40;
    case 13:
      k2 ^= uint64_t{tail_ptr[12]} << 32;
    case 12:
      k2 ^= uint64_t{tail_ptr[11]} << 24;
    case 11:
      k2 ^= uint64_t{tail_ptr[10]} << 16;
    case 10:
      k2 ^= uint64_t{tail_ptr[9]} << 8;
    case 9:
      k2 ^= uint64_t{tail_ptr[8]};
      k2 *= c2_128;
      rotl(k2, 33);
      k2 *= c1_128;
      h2 ^= k2;
    case 8:
      k1 ^= uint64_t{tail_ptr[7]} << 56;
    case 7:
      k1 ^= uint64_t{tail_ptr[6]} << 48;
    case 6:
      k1 ^= uint64_t{tail_ptr[5]} << 40;
    case 5:
      k1 ^= uint64_t{tail_ptr[4]} << 32;
    case 4:
      k1 ^= uint64_t{tail_ptr[3]} << 24;
    case 3:
      k1 ^= uint64_t{tail_ptr[2]} << 16;
    case 2:
      k1 ^= uint64_t{tail_ptr[1]} << 8;
    case 1:
      k1 ^= uint64_t{tail_ptr[0]} << 0;
      k1 *= c1_128;
      rotl(k1, 31);
      k1 *= c2_128;
      h1 ^= k1;
    }
    h1 ^= uint64_t{length};
    h2 ^= uint64_t{length};

    h1 += h2;
    h2 += h1;

    fmix64(h1);
    fmix64(h2);

    h1 += h2;
    h2 += h1;
    _out1 = h1;
    _out2 = h2;
  }

  void reset() {
    _h1 = 0;
    _h2 = 0;
  }

public:
  template <typename Iter_t>
  std::array<uint64_t, 4> sum256(Iter_t begin, Iter_t end) {
    const auto *ptr = reinterpret_cast<const uint8_t *>(begin);
    size_t len = std::distance(ptr, reinterpret_cast<const uint8_t *>(end));
    reset();
    std::array<uint64_t, 4> ret;
    size_t blocks = len / block_size;
    size_t tail_len = len % block_size;
    const uint8_t *tail_ptr = ptr + (blocks * block_size);

    bmix(ptr, blocks);
    sum128<false>(len, tail_ptr, tail_len, ret[0], ret[1]);

    if (tail_len + 1 == block_size) {
      uint64_t k1, k2;
      memcpy(&k1, tail_ptr, 8);
      k1 = htole64(k1);
      uint32_t l;
      memcpy(&l, tail_ptr + 8, 4);
      k2 = htole32(l);
      k2 |= (uint64_t{tail_ptr[12]} << 32) | (uint64_t{tail_ptr[13]} << 40) |
            (uint64_t{tail_ptr[14]} << 48);
      k2 |= uint64_t{1} << 56;
      bmix_words(k1, k2);
      sum128<false>(len + 1, tail_ptr, tail_len, ret[2], ret[3]);
    } else {
      sum128<true>(len + 1, tail_ptr, tail_len, ret[2], ret[3]);
    }
    return ret;
  }

  template <typename Iter_t>
  static std::array<uint64_t, 4> hash256(Iter_t begin, Iter_t end) {
    Digest d{};
    return d.sum256(begin, end);
  }
};

} // namespace murmur3
