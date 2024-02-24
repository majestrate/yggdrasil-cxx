
#include "event_base.hpp"
#include "sockaddr.hpp"

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

#include <algorithm>
#include <deque>
#include <liburing.h>

#include <spdlog/spdlog.h>

namespace {
/// c/s queue size in entries
constexpr uint32_t num_q_entries = 512;

} // namespace

namespace yggdrasil {

struct NodeInfo {
  std::array<uint8_t, 32> pubkey;

  in6_addr addr;
};

using BlockReader_t = Reader;

/// all our iops we can do allocated in one place
struct Resources {

  Resource<Accepter, 8> accepting;
  Resource<Closer, 8> closing;
  Resource<BlockReader_t, 128> reading;

  Resources() = default;
  Resources(const Resources &) = delete;
  Resources(Resources &&) = delete;
};

class State {
  Resources &_res;

public:
  std::pmr::deque<Accepter> accepting;
  std::pmr::deque<BlockReader_t> reading;
  std::pmr::deque<Closer> closing;

  int server_fd;

  bool enabled{true};

  State(Resources &res)

      : _res{res}, accepting{_res.accepting.allocator()},
        reading{_res.reading.allocator()}, closing{res.closing.allocator()} {}

  void bind_socket(const SockAddr &saddr) {
    server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
      throw std::runtime_error{fmt::format("socket(): {}", strerror(errno))};

    if (auto err = ::bind(server_fd, saddr.c_ptr(), saddr.socklen()); err == -1)
      throw std::runtime_error{fmt::format("bind(): {}", strerror(errno))};

    if (auto err = ::listen(server_fd, 5); err == -1)
      throw std::runtime_error{fmt::format("listen(): {}", strerror(errno))};

    accepting.emplace_back(server_fd);
  }

  void recv_msg(Accepter *ev, int fd) {
    spdlog::debug("accepted fd={}", fd);
    // connection enters reading mode
    reading.emplace_back(fd, size_t{128});

    // remove existing accept
    std::remove(accepting.begin(), accepting.end(), *ev);

    // accept another connection
    accepting.emplace_back(server_fd);
  }

  void recv_msg(BlockReader_t *ev, ssize_t num) {
    int fd = ev->fd();
    spdlog::debug("read {} bytes on fd={}", num, fd);

    std::remove(reading.begin(), reading.end(), *ev);
    /// if we read nothing we should close
    if (num <= 0) {
      closing.emplace_back(fd);
    } else
      reading.emplace_back(fd, size_t{128});
  }

  void recv_msg(Closer *ev, int) {
    int fd = ev->fd;
    std::remove(closing.begin(), closing.end(), *ev);

    if (fd == server_fd)
      end();
  }

  void close_server() { closing.emplace_back(server_fd); }

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
      spdlog::debug("io_uring_wait_cqe(): ret={} cqe={}", ret,
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
          state.recv_msg(ev, res);
        } else if (auto *ev = dynamic_cast<BlockReader_t *>(ptr)) {
          state.recv_msg(ev, res);
        } else if (auto *ev = dynamic_cast<Closer *>(ptr)) {
          state.recv_msg(ev, res);
        } else {
          // spdlog::info("no event found");
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
    state->close_server();
  }
  if (sig == SIGWINCH) {

    return;
  }
}

int main(int argc, char **argv) {
  auto s = signal(SIGINT, sig_handler);
  signal(SIGWINCH, s);

  io_uring_queue_init(num_q_entries, &yggdrasil::g_ring, 0);

  yggdrasil::Resources res{};

  yggdrasil::Loop event_loop{};

  yggdrasil::SockAddr listen_addr{"127.0.0.1", 5555};

  spdlog::info("starting up");
  state = std::make_unique<yggdrasil::State>(res);
  try {
    state->bind_socket(listen_addr);
  } catch (std::exception &ex) {
    spdlog::error("startup failed: {}", ex.what());
    return 1;
  }
  spdlog::info("running");
  try {
    event_loop.run(*state);
  } catch (std::exception &ex) {
    spdlog::error("event loop: {}", ex.what());
    return 1;
  }

  spdlog::info("stopped");

  return 0;
}
