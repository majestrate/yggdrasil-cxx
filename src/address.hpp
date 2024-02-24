#pragma once

namespace yggdrasil {

/// represents an IPv6 address in the yggdrasil address range.
using ip6_address_t = std::array<uint8_t, 16>;
/// represents an IPv6 /64 subnet in the yggdrasil subnet range.
using ip6_subnet_t = std::array<uint8_t, 8>;

/// returns true if this address holds an yggdrasil address.
bool is_valid(const ip6_address_t &addr);
/// returns true if this address holds an yggdrasil address.
bool is_valid(const ip6_subnet_t &subnet);

} // namespace yggdrasil