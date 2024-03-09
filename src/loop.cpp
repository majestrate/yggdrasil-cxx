#include <yggdrasil/core.hpp>
#include <yggdrasil/loop.hpp>

#include <spdlog/spdlog.h>

namespace yggdrasil {

void run_loop(State &state) {
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

    {
      auto res = cqe->res;
      auto *ptr = reinterpret_cast<EventBase *>(io_uring_cqe_get_data(cqe));
      if (ptr)
        ptr->completion(state, res);
    }
    io_uring_cqe_seen(&g_ring, cqe);
  }
}
} // namespace yggdrasil