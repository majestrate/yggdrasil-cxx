#include <yggdrasil/varint.hpp>
#include <catch2/catch.hpp>




TEST_CASE( "Varint Encodes correctly", "[varint]" ) {
    std::array<uint8_t, yggdrasil::max_varuint_64bit_bytes> data;


   yggdrasil::write_golang_varuint(300, data.begin(), data.end());

	REQUIRE(int{data[0]} == 0xac);
	REQUIRE(int{data[1]} == 0x01);

	auto [x, itr] = yggdrasil::read_golang_varuint(data.begin(), data.end());

	REQUIRE(x == 9); 
}