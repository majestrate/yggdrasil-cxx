
#include "events.hpp"
#include "utils.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <vector>

#include <deque>

#include <any>
#include <spdlog/spdlog.h>

#include <liburing.h>

#include <memory_resource>

namespace {
/// c/s queue size in entries
constexpr uint32_t num_q_entries = 512;

} // namespace

extern io_uring g_ring;

io_uring *ring_ptr{};

namespace yggdrasil {

struct NodeInfo {
  std::array<uint8_t, 32> pubkey;

  in6_addr addr;
};

/// reads a handshake for exchanging routing information on an established FD
class HandshakeReader {
  std::array<uint8_t, 32> buffer;
  iovec vec;
  sockaddr_in saddr;

public:
  HandshakeReader() = default;
  HandshakeReader(int fd, const sockaddr_in &addr) : saddr{addr} {
    vec.iov_base = buffer.data();
    vec.iov_len = buffer.size();
    auto *sqe = io_uring_get_sqe(ring_ptr);
    io_uring_prep_readv(sqe, fd, &vec, 1, 0);
    io_uring_sqe_set_data(sqe, this);
    io_uring_submit(ring_ptr);
  }
};

/// accept a single inbound conections and does a handshake on them
class Accepter {
  sockaddr_in saddr;
  socklen_t slen;

public:
  bool constexpr operator==(const Accepter &other) const {
    return saddr.sin_addr.s_addr == other.saddr.sin_addr.s_addr and
           saddr.sin_port == other.saddr.sin_port;
  }

  Accepter() = default;

  Accepter(int server_socket) {
    auto *sqe = io_uring_get_sqe(ring_ptr);
    io_uring_prep_accept(sqe, server_socket,
                         reinterpret_cast<sockaddr *>(&saddr), &slen, 0);
    io_uring_sqe_set_data(sqe, this);
    io_uring_submit(ring_ptr);
  }

  ~Accepter() = default;

  constexpr const sockaddr_in &address() const { return saddr; };
};

struct TransitTraffic {
  std::array<uint8_t, 1024 * 8> read_buffer;
};

template <typename T, size_t ents> struct Resource {
  std::pmr::monotonic_buffer_resource _mem{sizeof(T) * ents};
  std::pmr::polymorphic_allocator<T> _alloc{&_mem};
  constexpr const auto &mem() const { return _alloc; };
};

/// all our iops we can do allocated in one place
struct Resources {

  Resource<Accepter, 8> accepter;
  Resource<HandshakeReader, 16> handshaker;
  Resource<TransitTraffic, 128> transiting;

  Resources() = default;
  Resources(const Resources &) = delete;
  Resources(Resources &&) = delete;
};

class State {
  Resources _res;

  std::pmr::deque<Accepter> accepting{_res.accepter.mem()};
  std::pmr::deque<HandshakeReader> handshaking{_res.handshaker.mem()};
  std::pmr::deque<TransitTraffic> transiting{_res.transiting.mem()};

  int server_fd;

public:
  State(SockAddr saddr) {
    server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
      throw std::runtime_error{fmt::format("socket(): {}", strerror(errno))};

    if (auto err = ::bind(server_fd, saddr.ptr(), saddr.socklen()); err == -1)
      throw std::runtime_error{fmt::format("bind(): {}", strerror(errno))};

    if (auto err = ::listen(server_fd, 5); err == -1)
      throw std::runtime_error{fmt::format("listen(): {}", strerror(errno))};

    spdlog::info("accepting on {}", saddr.str());
    accepting.emplace_back(server_fd);
  }

  void recv_inbound_handshake(Accepter *ev, int fd) {
    spdlog::info("got new inbound");
    handshaking.emplace_back(fd, ev->address());
    std::remove(accepting.begin(), accepting.end(), *ev);
  }
};

class Loop {
public:
  void run(State &state) const {
    io_uring_cqe *cqe;
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
      int ret = io_uring_wait_cqe(ring_ptr, &cqe);

      // interrupt requested
      if (ret == 0 - EINTR)
        return;

      if (ret < 0)
        throw std::runtime_error{
            fmt::format("io_uring_wait_cqe(): {}", strerror(0 - ret))};

      if (auto res = cqe->res; res >= 0) {
        auto *ptr = reinterpret_cast<std::any *>(io_uring_cqe_get_data(cqe));

        if (Accepter *ev = std::any_cast<Accepter>(ptr)) {
          // accept was good.
          state.recv_inbound_handshake(ev, res);
        } else if (HandshakeReader *ev = std::any_cast<HandshakeReader>(ptr)) {
          // handshake read happened.

        } else {
          spdlog::info("no event found");
        }
      }
      io_uring_cqe_seen(ring_ptr, cqe);
    }
  }
};

} // namespace yggdrasil

void sigint_handler(int) {
  spdlog::info("shutting down");
  io_uring_queue_exit(ring_ptr);
}

int main(int argc, char **argv) {

  io_uring g_ring;
  ring_ptr = &g_ring;
  signal(SIGINT, sigint_handler);
  io_uring_queue_init(num_q_entries, ring_ptr, 0);

  auto resources = std::make_unique<yggdrasil::Resources>();

  yggdrasil::Loop event_loop{};

  yggdrasil::SockAddr listen_addr{"127.0.0.1", 5555};

  try {

    yggdrasil::State state{listen_addr};
    event_loop.run(state);
  } catch (std::exception &ex) {
    spdlog::error("event loop: {}", ex.what());
    return 1;
  }
  spdlog::info("stopped");

  return 0;
}
