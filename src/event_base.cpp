#include <stdexcept>
#include <yggdrasil/core.hpp>
#include <yggdrasil/event_base.hpp>

#include <fmt/format.h>

namespace yggdrasil {

Accepter::Accepter(int server_socket) : _fd{server_socket} {
  auto *sqe = get_sqe(&g_ring, this);
  io_uring_prep_accept(sqe, fd(), addr.ptr(), &slen, SOCK_NONBLOCK);
  io_uring_submit(&g_ring);
}

void Accepter::completion(State &st, int res) {
  _cfd = res;
  st.event(*this);
}

Accepter::~Accepter() { _fd = -1; };

Reader::Reader(int fd, size_t n) : _fd{fd} {
  if (n > buf.size())
    throw std::overflow_error{
        fmt::format("cannot read, {} >= {}", n, buf.size())};
  vec.iov_base = buf.data();
  vec.iov_len = n;
  auto *sqe = get_sqe(&g_ring, this);
  io_uring_prep_readv(sqe, _fd, &vec, 1, 0);
  io_uring_submit(&g_ring);
}

void Reader::completion(State &st, int res) {
  vec.iov_len = res;
  st.event(*this);
}

Reader::~Reader() { _fd = -1; };

byte_view_t Reader::data() const {
  return byte_view_t{reinterpret_cast<const uint8_t *>(vec.iov_base),
                     vec.iov_len};
}

Closer::Closer(int fd_) : _fd{fd_} {
  auto *sqe = get_sqe(&g_ring, this);
  io_uring_prep_close(sqe, fd());
  io_uring_submit(&g_ring);
}

void Closer::completion(State &st, int) { st.event(*this); }

Closer::~Closer() { _fd = -1; };

::io_uring_sqe *get_sqe(io_uring *ring, void *self) {
  auto *sqe = io_uring_get_sqe(ring);
  io_uring_sqe_set_data(sqe, self);
  return sqe;
}

io_uring g_ring;
} // namespace yggdrasil
