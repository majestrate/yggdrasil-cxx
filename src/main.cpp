
#include "utils.hpp"

#include <array>
#include <concepts>
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
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <liburing.h>

#include <memory_resource>

namespace {
/// c/s queue size in entries
constexpr uint32_t num_q_entries = 512;

} // namespace

io_uring g_ring;

namespace yggdrasil {

struct NodeInfo {
  std::array<uint8_t, 32> pubkey;

  in6_addr addr;
};

struct EventBase {
  virtual ~EventBase() = default;
  EventBase() = default;
};

template <typename T>
constexpr bool is_event_v = std::derived_from<T, EventBase>;

/// gets a submission queue event and associates a T* with it
template <typename T> auto *get_sqe(io_uring *ring, T *self) {
  static_assert(is_event_v<T>);
  auto *sqe = io_uring_get_sqe(ring);
  io_uring_sqe_set_data(sqe, self);
  return sqe;
}

/// reads a data on an established FD
template <typename T = std::array<uint8_t, 128>, size_t N = 1>
struct Reader : public EventBase {
  std::array<T, N> datas;
  std::array<iovec, N> vecs;
  int fd;

  Reader() : EventBase{} {}

  explicit Reader(int fd_) : EventBase{}, fd{fd_} {
    for (size_t idx = 0; idx < N; ++idx) {
      vecs[idx].iov_base = datas[idx].data();
      vecs[idx].iov_len = datas[idx].size();
    }
    auto *sqe = get_sqe(&g_ring, this);
    io_uring_prep_readv(sqe, fd, vecs.data(), vecs.size(), 0);
    io_uring_submit(&g_ring);
  }

  bool constexpr operator==(const Reader &other) const {
    return fd == other.fd;
  }
  ~Reader() { fd = -1; }
};

/// accept a single inbound conections and does a handshake on them
class Accepter : public EventBase {

  int fd;

public:
  bool operator==(const Accepter &other) const { return fd == other.fd; }

  Accepter() : EventBase{} {};

  explicit Accepter(int server_socket) : EventBase{}, fd{server_socket} {
    auto *sqe = get_sqe(&g_ring, this);
    io_uring_prep_accept(sqe, server_socket, nullptr, nullptr, SOCK_NONBLOCK);
    io_uring_submit(&g_ring);
  }

  ~Accepter() = default;
};

/// closes an open file handle
struct Closer : public EventBase {

  int fd;
  bool constexpr operator==(const Closer &other) const {
    return fd == other.fd;
  }

  Closer() : EventBase{} {};

  explicit Closer(int fd_) : EventBase{}, fd{fd_} {
    auto *sqe = get_sqe(&g_ring, this);
    io_uring_prep_close(sqe, fd);
    io_uring_submit(&g_ring);
  }

  ~Closer() { fd = -1; }
};

template <typename T, size_t ents> struct Resource {
  static_assert(is_event_v<T>);
  std::pmr::monotonic_buffer_resource _mem{sizeof(T) * ents};
  std::pmr::polymorphic_allocator<T> _alloc{&_mem};
  constexpr const auto &mem() const { return _alloc; };
};

using BlockReader_t = Reader<std::array<uint8_t, 128>>;

/// all our iops we can do allocated in one place
struct Resources {

  Resource<Accepter, 8> accepter;
  Resource<Closer, 8> closers;
  Resource<BlockReader_t, 128> reading;

  Resources() = default;
  Resources(const Resources &) = delete;
  Resources(Resources &&) = delete;
};

class State {
  Resources _res;

  std::pmr::deque<Accepter> accepting{_res.accepter.mem()};
  std::pmr::deque<BlockReader_t> reading{_res.reading.mem()};
  std::pmr::deque<Closer> closing{_res.closers.mem()};

  int server_fd;

public:
  bool enabled{true};

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

  void recv_inbound_connection(Accepter *ev, int fd) {
    spdlog::info("new inbound connection, fd={}", fd);
    // connection enters reading mode
    reading.emplace_back(fd);

    // remove existing accept
    std::remove(accepting.begin(), accepting.end(), *ev);

    // accept another connection
    accepting.emplace_back(server_fd);
  }

  void recv_data(BlockReader_t *ev, ssize_t num) {

    int fd = ev->fd;
    spdlog::info("read {} bytes on fd={} {}", num, fd,
                 spdlog::to_hex(ev->datas[0]));

    std::remove(reading.begin(), reading.end(), *ev);
    /// if we read nothing we should close
    if (num <= 0) {
      closing.emplace_back(fd);
    } else
      reading.emplace_back(fd);
  }

  void end() {
    io_uring_queue_exit(&g_ring);
    enabled = false;
  }
};

class Loop {
public:
  void run(State &state) const {
    io_uring_cqe *cqe;

    while (state.enabled) {
      int ret = io_uring_wait_cqe(&g_ring, &cqe);
      spdlog::info("io_uring_wait_cqe(): ret={} cqe={}", ret,
                   cqe == nullptr ? "null" : "not-null");

      if (ret < 0 and ret != 0 - EINTR)
        throw std::runtime_error{
            fmt::format("io_uring_wait_cqe(): {}", strerror(0 - ret))};

      if (cqe == nullptr)
        continue;

      auto res = cqe->res;
      {
        auto *ptr = reinterpret_cast<EventBase *>(io_uring_cqe_get_data(cqe));

        if (auto *ev = dynamic_cast<Accepter *>(ptr)) {
          // accept was good.
          state.recv_inbound_connection(ev, res);
        } else if (auto *ev = dynamic_cast<BlockReader_t *>(ptr)) {
          // read happened.
          state.recv_data(ev, res);
        } else if (auto *ev = dynamic_cast<Closer *>(ptr)) {
          spdlog::info("close() on fd={} res={}", ev->fd, res);
        } else {
          spdlog::info("no event found");
        }
      }
      io_uring_cqe_seen(&g_ring, cqe);
    }
  }
};

} // namespace yggdrasil

std::unique_ptr<yggdrasil::State> state;

void sig_handler(int sig) {
  if (sig == SIGINT or sig == SIGTERM) {
    spdlog::info("shutting down");
    state->end();
  }
  if (sig == SIGWINCH) {

    return;
  }
}

int main(int argc, char **argv) {
  auto s = signal(SIGINT, sig_handler);
  signal(SIGWINCH, s);

  io_uring_queue_init(num_q_entries, &g_ring, 0);

  auto resources = std::make_unique<yggdrasil::Resources>();

  yggdrasil::Loop event_loop{};

  yggdrasil::SockAddr listen_addr{"127.0.0.1", 5555};

  try {
    state = std::make_unique<yggdrasil::State>(listen_addr);
    event_loop.run(*state);
  } catch (std::exception &ex) {
    spdlog::error("event loop: {}", ex.what());
    return 1;
  }
  spdlog::info("stopped");

  return 0;
}
