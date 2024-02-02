
#include "event_base.hpp"
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

namespace {
/// c/s queue size in entries
constexpr uint32_t num_q_entries = 512;

} // namespace

namespace yggdrasil {

struct NodeInfo {
  std::array<uint8_t, 32> pubkey;

  in6_addr addr;
};

using BlockReader_t = Reader<std::array<uint8_t, 128>>;

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

  State(Resources &res, SockAddr saddr)
      : _res{res}, accepting{_res.accepting.mem()}, reading{_res.reading.mem()},
        closing{res.closing.mem()} {
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

  io_uring_queue_init(num_q_entries, &yggdrasil::g_ring, 0);

  yggdrasil::Resources res{};

  yggdrasil::Loop event_loop{};

  yggdrasil::SockAddr listen_addr{"127.0.0.1", 5555};

  try {
    state = std::make_unique<yggdrasil::State>(res, listen_addr);
    event_loop.run(*state);
  } catch (std::exception &ex) {
    spdlog::error("event loop: {}", ex.what());
    return 1;
  }
  spdlog::info("stopped");

  return 0;
}
