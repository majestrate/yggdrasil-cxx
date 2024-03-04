#pragma once

#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

namespace yggdrasil {

struct SockAddr {
  sockaddr_storage _data;

  SockAddr() = default;
  SockAddr(std::string ip, uint16_t port);

  // explicit SockAddr(sockaddr *);

  /// get as const sockaddr *
  const sockaddr *c_ptr() const;
  /// get as sockaddr *
  sockaddr *ptr();

  /// get address family
  int fam() const;

  /// get socklen_t
  socklen_t socklen() const;

  /// printable string
  std::string str() const;

  bool operator==(const SockAddr &other) const;
};

inline bool operator==(const in_addr &lhs, const in_addr &rhs) {
  return memcmp(&lhs, &rhs, sizeof(in_addr)) == 0;
}

inline bool operator==(const sockaddr_in &lhs, const sockaddr_in &rhs) {
  return lhs.sin_addr == rhs.sin_addr and lhs.sin_port == rhs.sin_port;
}

inline bool operator==(const in6_addr &lhs, const in6_addr &rhs) {
  return memcmp(&lhs, &rhs, sizeof(in6_addr)) == 0;
}

inline bool operator==(const sockaddr_in6 &lhs, const sockaddr_in6 &rhs) {
  return lhs.sin6_addr == rhs.sin6_addr and lhs.sin6_port == rhs.sin6_port;
}

std::string to_string(const sockaddr_in &addr);
std::string to_string(const sockaddr_in6 &addr);

} // namespace yggdrasil
