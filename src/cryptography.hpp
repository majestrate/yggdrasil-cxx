#pragma once

#include <cstdint>
#include <openssl/evp.h>

#include <array>

namespace yggdrasil {

/// represents an IPv6 address in the yggdrasil address range.
using ip6_address_t = std::array<uint8_t, 16>;
/// represents an IPv6 /64 subnet in the yggdrasil subnet range.
using ip6_subnet_t = std::array<uint8_t, 8>;

/// returns true if this address holds an yggdrasil address.
bool is_valid(const ip6_address_t &addr);
/// returns true if this address holds an yggdrasil address.
bool is_valid(const ip6_subnet_t &subnet);

/// converts a public ed25519 key to an ipv6 address
ip6_address_t addr_from_key(const std::array<uint8_t, 32> &pubkey);

/// converts a public ed25519 key to a subnet, which uses a different prefix.
ip6_subnet_t subnet_for_key(const std::array<uint8_t, 32> &pubkey);

/// takes in an ip6 address and will fill out half of the public so we can look
/// the full one up
void address_to_partial_pubkey(const ip6_address_t &addr,
                               std::array<uint8_t, 32> &pubkey);
} // namespace yggdrasil
