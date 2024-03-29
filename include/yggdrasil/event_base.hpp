#pragma once

#include <yggdrasil/buffer_ops.hpp>
#include <yggdrasil/sockaddr.hpp>

#include <liburing.h>
#include <sys/socket.h>

#include <array>
#include <concepts>
#include <memory_resource>

namespace yggdrasil {

extern ::io_uring g_ring;

// forward declare.
class State;

/// common type for all dispatchable events
/// we use rtti from a reinterpret_cast on a void* to an EventBase* that we
/// get from liburing's sqe data
class EventBase {
public:
  virtual ~EventBase() = default;
  /// @brief called on completion of event. mutates state.
  virtual void completion(State &st, int result) = 0;
};

/// a holder of N dispatchable events
template <typename T, size_t N> struct Resource {
  static_assert(std::derived_from<T, EventBase>);
  std::pmr::monotonic_buffer_resource _mem{sizeof(T) * N};
  std::pmr::polymorphic_allocator<T> _alloc{&_mem};
  /// get the allocator for this resource
  constexpr const auto &allocator() const { return _alloc; };
};

/// gets a submission queue event and associates a pointer with it
::io_uring_sqe *get_sqe(io_uring *ring, void *self);

/// reads a data on an established FD
class Reader : public EventBase {

  int _fd;
  iovec vec;
  /// underlying buffer
  std::array<std::byte, 4 * 1024> buf;

public:
  Reader() = default;
  explicit Reader(int fd_, size_t n);

  bool constexpr operator==(const Reader &other) const {
    return fd() == other.fd();
  }

  void completion(State &state, int result) override;

  virtual ~Reader();

  byte_view_t data() const;

  constexpr int fd() const { return _fd; }
};

/// accept a single inbound conections and does a handshake on them
class Accepter : public EventBase {

  int _fd;
  SockAddr addr;
  socklen_t slen;

public:
  constexpr int fd() const { return _fd; }
  bool constexpr operator==(const Accepter &other) const {
    return fd() == other.fd();
  }

  Accepter() = default;
  explicit Accepter(int server_socket);
  virtual ~Accepter();
  void completion(State &state, int result) override;
};

/// closes an open file handle
class Closer : public EventBase {
  int _fd;

public:
  bool constexpr operator==(const Closer &other) const {
    return fd() == other.fd();
  }

  constexpr int fd() const { return _fd; };

  Closer() = default;

  explicit Closer(int fd_);
  virtual ~Closer();
  void completion(State &state, int result) override;
};

} // namespace yggdrasil
