
#include <yggdrasil/core.hpp>
#include <yggdrasil/event_base.hpp>
#include <yggdrasil/loop.hpp>
#include <yggdrasil/sockaddr.hpp>

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
/// submission/completion queue size in entries
constexpr uint32_t num_q_entries = 512;

} // namespace

std::unique_ptr<yggdrasil::State> state;
std::unique_ptr<yggdrasil::CloseLoop> close_loop;

void sig_handler(int sig) {
  if (sig == SIGINT or sig == SIGTERM) {
    spdlog::info("shutting down");
    if (close_loop == nullptr)
      close_loop = std::make_unique<yggdrasil::CloseLoop>(*state);
  }
  if (sig == SIGWINCH) {

    return;
  }
}

int main(int argc, char **argv) {
  auto s = signal(SIGINT, sig_handler);
  signal(SIGWINCH, s);

  io_uring_queue_init(num_q_entries, &yggdrasil::g_ring, 0);

  yggdrasil::SockAddr listen_addr{"127.0.0.1", 5555};

  spdlog::info("starting up");

  auto res_ptr = yggdrasil::make_resources();

  state = std::make_unique<yggdrasil::State>(*res_ptr);
  try {
    state->bind_server_socket(listen_addr);
  } catch (std::exception &ex) {
    spdlog::error("startup failed: {}", ex.what());
    return 1;
  }
  spdlog::info("running");
  try {
    yggdrasil::run_loop(*state);
  } catch (std::exception &ex) {
    spdlog::error("event loop: {}", ex.what());
    return 1;
  }

  spdlog::info("stopped");

  return 0;
}
