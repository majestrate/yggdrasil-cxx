#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <yggdrasil/core.hpp>

namespace yggdrasil {

/// all our iops we can do allocated in one place
struct Resources {

  Resource<Accepter, 8> accepting;
  Resource<Closer, 8> closing;
  Resource<Reader, 128> reading;

  Resources() = default;
  Resources(const Resources &) = delete;
  Resources(Resources &&) = delete;
};

std::shared_ptr<Resources> make_resources() {
  return std::make_shared<Resources>();
}

State::State(Resources &res)
    : _res{res}, accepting{_res.accepting.allocator()},
      reading{_res.reading.allocator()}, closing{res.closing.allocator()} {}

void State::bind_server_socket(const SockAddr &saddr) {
  server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1)
    throw std::runtime_error{fmt::format("socket(): {}", strerror(errno))};

  if (auto err = ::bind(server_fd, saddr.c_ptr(), saddr.socklen()); err == -1)
    throw std::runtime_error{fmt::format("bind(): {}", strerror(errno))};

  if (auto err = ::listen(server_fd, 5); err == -1)
    throw std::runtime_error{fmt::format("listen(): {}", strerror(errno))};

  accepting.emplace_back(server_fd);
}

void State::recv_msg(Accepter *ev, int fd) {
  spdlog::debug("accepted fd={}", fd);
  // connection enters reading mode
  reading.emplace_back(fd, size_t{128});

  // remove existing accept
  std::remove(accepting.begin(), accepting.end(), *ev);

  // accept another connection
  accepting.emplace_back(server_fd);
}

void State::recv_msg(Reader *ev, ssize_t num) {
  int fd = ev->fd();
  spdlog::debug("read {} bytes on fd={}", num, fd);

  std::remove(reading.begin(), reading.end(), *ev);
  // if we read nothing we should close
  if (num <= 0) {
    closing.emplace_back(fd);
  } else
    reading.emplace_back(fd, size_t{128});
}

void State::recv_msg(Closer *ev) {
  if (ev->fd() == server_fd)
    end();
  std::remove(closing.begin(), closing.end(), *ev);
}

void State::close_server_socket() { closing.emplace_back(server_fd); }

void State::end() {
  io_uring_queue_exit(&g_ring);
  enabled = false;
}

} // namespace yggdrasil
