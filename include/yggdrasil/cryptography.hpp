#pragma once

#include <cstdint>
#include <openssl/evp.h>

#include <array>

#include <yggdrasil/address.hpp>

namespace yggdrasil {

using ed25519_pubkey_t = std::array<uint8_t, 32>;

/// converts a public ed25519 key to an ipv6 address
ip6_address_t addr_from_key(const ed25519_pubkey_t &pubkey);

/// converts a public ed25519 key to a subnet, which uses a different prefix.
ip6_subnet_t subnet_for_key(const ed25519_pubkey_t &pubkey);

/// takes in an ip6 address and will fill out half of the public so we can look
/// the full one up
void address_to_partial_pubkey(const ip6_address_t &addr,
                               ed25519_pubkey_t &pubkey);
} // namespace yggdrasil
