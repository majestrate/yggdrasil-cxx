#include <catch2/catch.hpp>
#include <cstdint>
#include <yggdrasil/bloom.hpp>

#include <array>

TEST_CASE("Test bloom filter add", "[bloom]") {
  std::array<uint8_t, 32> pk{1};
  yggdrasil::Bloom bloom{};
  bloom.add_key(pk);
  REQUIRE(bloom.has_key(pk));
}

TEST_CASE("Test bloom filter encode 64bit", "[bloom]") {

  std::array<uint8_t, 32> pk{1};
  yggdrasil::Bloom bloom1{}, bloom2{}, bloom3{};
  bloom1.add_key(pk);

  std::array<uint64_t, yggdrasil::Bloom::u64_size()> tmp64;
  std::array<uint8_t, yggdrasil::Bloom::byte_size()> tmp;

  REQUIRE(not bloom2.has_key(pk));

  bloom1.encode(tmp64.begin(), tmp64.end());
  bloom1.encode(tmp.begin(), tmp.end());
  bloom2.decode(tmp64.begin(), tmp64.end());
  bloom3.decode(tmp.begin(), tmp.end());

  REQUIRE(bloom1.has_key(pk));
  REQUIRE(bloom2.has_key(pk));
  REQUIRE(bloom1 == bloom2);
  REQUIRE(bloom2 == bloom3);
}

TEST_CASE("Test bloom filter encode 32bit", "[bloom]") {

  std::array<uint8_t, 32> pk{1};
  yggdrasil::Bloom bloom1{}, bloom2{}, bloom3{};
  bloom1.add_key(pk);

  std::array<uint32_t, yggdrasil::Bloom::u64_size() * 2> tmp32;
  std::array<uint8_t, yggdrasil::Bloom::byte_size()> tmp;
  REQUIRE(not bloom2.has_key(pk));

  bloom1.encode(tmp32.begin(), tmp32.end());
  bloom1.encode(tmp.begin(), tmp.end());
  bloom2.decode(tmp32.begin(), tmp32.end());
  bloom3.decode(tmp.begin(), tmp.end());

  REQUIRE(bloom1.has_key(pk));
  REQUIRE(bloom2.has_key(pk));
  REQUIRE(bloom1 == bloom2);
  REQUIRE(bloom2 == bloom3);
}

TEST_CASE("Test bloom filter encode 16bit", "[bloom]") {

  std::array<uint8_t, 32> pk{1};
  yggdrasil::Bloom bloom1{}, bloom2{}, bloom3{};
  bloom1.add_key(pk);

  std::array<uint16_t, yggdrasil::Bloom::byte_size() / 2> tmp16;
  std::array<uint8_t, yggdrasil::Bloom::byte_size()> tmp;
  REQUIRE(not bloom2.has_key(pk));

  bloom1.encode(tmp16.begin(), tmp16.end());
  bloom1.encode(tmp.begin(), tmp.end());
  bloom2.decode(tmp16.begin(), tmp16.end());
  bloom3.decode(tmp.begin(), tmp.end());

  REQUIRE(bloom1.has_key(pk));
  REQUIRE(bloom2.has_key(pk));
  REQUIRE(bloom1 == bloom2);
  REQUIRE(bloom2 == bloom3);
}

TEST_CASE("Test bloom filter encode 8bit", "[bloom]") {

  std::array<uint8_t, 32> pk{1};
  yggdrasil::Bloom bloom1{}, bloom2{};
  bloom1.add_key(pk);

  std::array<uint8_t, yggdrasil::Bloom::byte_size()> tmp;

  REQUIRE(not bloom2.has_key(pk));

  bloom1.encode(tmp.begin(), tmp.end());
  bloom2.decode(tmp.begin(), tmp.end());

  REQUIRE(bloom1.has_key(pk));
  REQUIRE(bloom2.has_key(pk));
  REQUIRE(bloom1 == bloom2);
}

TEST_CASE("Test BloomState", "[bloom]") {
  using Pubkey = std::array<uint8_t, 32>;
  std::pmr::unsynchronized_pool_resource mempool;
  std::pmr::polymorphic_allocator<Pubkey> key_alloc{&mempool};
  std::pmr::polymorphic_allocator<yggdrasil::Bloom> bloom_alloc{&mempool};
  yggdrasil::BloomState<Pubkey> state{key_alloc, bloom_alloc};
  bool reached{false};
  Pubkey pk{1};
  yggdrasil::Bloom send, recv;
  state.add_state(pk, send, recv, true, false);
  state.for_each([pk, &reached](auto &key, auto &send, auto &recv, auto ontree,
                                auto zdirty) {
    reached = true;
    REQUIRE(pk == key);
    REQUIRE(ontree == true);
    REQUIRE(zdirty == false);
  });
  REQUIRE(reached);
}
