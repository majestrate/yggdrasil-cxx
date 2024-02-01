#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>

namespace yggdrasil {

struct SockAddr {
  sockaddr_storage _data;

  SockAddr() = default;
  SockAddr(std::string ip, uint16_t port);

  explicit SockAddr(sockaddr *);

  /// get as sockaddr *
  const sockaddr *ptr() const;

  /// get address family
  int fam() const;

  /// get socklen_t
  socklen_t socklen() const;

  /// printable string
  std::string str() const;
};
} // namespace yggdrasil
