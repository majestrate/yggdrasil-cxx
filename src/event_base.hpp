#pragma once

#include "buffer_ops.hpp"
#include "sockaddr.hpp"
#include <liburing.h>

#include <array>
#include <concepts>

#include <memory_resource>
#include <sys/socket.h>

namespace yggdrasil {

extern ::io_uring g_ring;

class EventBase {
public:
  virtual ~EventBase() = default;
};

template <typename T>
constexpr bool is_event_v = std::derived_from<T, EventBase>;

template <typename T, size_t ents> struct Resource {
  static_assert(is_event_v<T>);
  std::pmr::monotonic_buffer_resource _mem{sizeof(T) * ents};
  std::pmr::polymorphic_allocator<T> _alloc{&_mem};
  constexpr const auto &mem() const { return _alloc; };
};

/// gets a submission queue event and associates a pointer with it
static auto *get_sqe(io_uring *ring, void *self) {
  auto *sqe = io_uring_get_sqe(ring);
  io_uring_sqe_set_data(sqe, self);
  return sqe;
}

/// reads a data on an established FD
class Reader : public EventBase {

  /// underlying buffer
  std::array<std::byte, 4 * 1028> buf;
  iovec vec;
  int _fd;

public:
  Reader() = default;

  explicit Reader(int fd_, size_t n);

  bool constexpr operator==(const Reader &other) const {
    return fd() == other.fd();
  }

  virtual ~Reader() { _fd = -1; }

  byte_view_t data() const;

  constexpr int fd() const { return _fd; }
};

/// accept a single inbound conections and does a handshake on them
class Accepter : public EventBase {

  int fd;
  SockAddr addr;
  socklen_t slen;

public:
  bool constexpr operator==(const Accepter &other) const {
    return fd == other.fd;
  }

  Accepter() = default;

  explicit Accepter(int server_socket);
  virtual ~Accepter() { fd = -1; };
};

/// closes an open file handle
struct Closer : public EventBase {

  int fd;
  bool constexpr operator==(const Closer &other) const {
    return fd == other.fd;
  }

  Closer() = default;

  explicit Closer(int fd_) : fd{fd_} {
    auto *sqe = get_sqe(&g_ring, this);
    io_uring_prep_close(sqe, fd);
    io_uring_submit(&g_ring);
  }

  virtual ~Closer() { fd = -1; }
};

} // namespace yggdrasil
