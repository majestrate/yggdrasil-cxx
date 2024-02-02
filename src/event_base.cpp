#include "event_base.hpp"

namespace yggdrasil {

Accepter::Accepter(int server_socket) : fd{server_socket} {
  auto *sqe = get_sqe(&g_ring, this);
  io_uring_prep_accept(sqe, fd, addr.ptr(), &slen, SOCK_NONBLOCK);
  io_uring_submit(&g_ring);
}

io_uring g_ring;
} // namespace yggdrasil
