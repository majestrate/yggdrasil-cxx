#include "utils.hpp"
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <netinet/in.h>
#include <stdexcept>

namespace yggdrasil {

SockAddr::SockAddr(std::string ip, uint16_t port) {
  auto *ptr_v4 = reinterpret_cast<sockaddr_in *>(&_data);
  if (auto res = inet_pton(AF_INET, ip.c_str(), &ptr_v4->sin_addr); res == 1) {
    ptr_v4->sin_family = AF_INET;
    ptr_v4->sin_port = htons(port);
    return;
  }
  auto *ptr_v6 = reinterpret_cast<sockaddr_in6 *>(&_data);
  if (auto res = inet_pton(AF_INET6, ip.c_str(), &ptr_v6->sin6_addr);
      res == 1) {
    ptr_v6->sin6_family = AF_INET6;
    ptr_v6->sin6_port = htons(port);
    return;
  }
  throw std::invalid_argument{
      fmt::format("invalid address: '{}:{}'", ip, port)};
}

/// get as sockaddr *
const sockaddr *SockAddr::ptr() const {
  return reinterpret_cast<const sockaddr *>(&_data);
}

/// get address family
int SockAddr::fam() const { return ptr()->sa_family; }

/// get socklen_t
socklen_t SockAddr::socklen() const {
  static constexpr const auto _lookup = std::to_array({
      std::make_pair(AF_INET, sizeof(sockaddr_in)),
      std::make_pair(AF_INET6, sizeof(sockaddr_in6)),
  });
  auto _fam{fam()};

  for (const auto &ent : _lookup) {
    const auto &[fam, slen] = ent;
    if (fam == _fam)
      return slen;
  }

  throw std::runtime_error{fmt::format("invalid address family: {}", _fam)};
}

std::string SockAddr::str() const {
  std::array<char, 128> buf{};
  if (fam() == AF_INET) {
    auto *addr = reinterpret_cast<const sockaddr_in *>(&_data);
    inet_ntop(AF_INET, &addr->sin_addr, buf.data(), buf.size());
    return fmt::format("{}:{}", buf.data(), ntohs(addr->sin_port));
  }
  throw std::runtime_error{fmt::format("invalid address family: {}", fam())};
}
} // namespace yggdrasil
