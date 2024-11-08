#include <catch2/catch.hpp>
#include <yggdrasil/varint.hpp>

#include <array>

TEST_CASE("Varint Encodes correctly", "[varint]") {
  std::array<uint8_t, yggdrasil::max_varuint_64bit_bytes> data{};

  {
    auto itr = yggdrasil::write_golang_varuint(300, data.begin(), data.end());

    REQUIRE(int{data[0]} == 0xac);
    REQUIRE(int{data[1]} == 0x82);
    REQUIRE(int{data[2]} == 0x0);

    REQUIRE(itr == data.begin() + 2);
  }
  {
    auto [x, itr] = yggdrasil::read_golang_varuint(data.begin(), data.end());

    REQUIRE(x == 300);
    REQUIRE(itr == data.begin() + 2);
  }
}
