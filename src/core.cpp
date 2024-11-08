#include "yggdrasil/connection.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <yggdrasil/core.hpp>
#include <yggdrasil/varint.hpp>

namespace yggdrasil {

/// all our iops we can do allocated in one place
struct Resources {

  Resource<Accepter, 8> accepting;
  Resource<Closer, 8> closing;
  Resource<Reader, 128> reading;
  DynResource<ConnectionState> conns;

  Resources() = default;
  Resources(const Resources &) = delete;
  Resources(Resources &&) = delete;
};

std::shared_ptr<Resources> make_resources() {
  return std::make_shared<Resources>();
}

State::State(Resources &res)
    : _res{res}, conns{_res.conns.allocator()},
      accepting{_res.accepting.allocator()}, reading{_res.reading.allocator()},
      closing{_res.closing.allocator()} {}

void State::bind_server_socket(const SockAddr &saddr) {
  server_fd = ::socket(saddr.fam(), SOCK_STREAM, 0);
  if (server_fd == -1)
    throw std::runtime_error{fmt::format("socket(): {}", strerror(errno))};

  if (auto err = ::bind(server_fd, saddr.c_ptr(), saddr.socklen()); err == -1)
    throw std::runtime_error{fmt::format("bind(): {}", strerror(errno))};

  if (auto err = ::listen(server_fd, 5); err == -1)
    throw std::runtime_error{fmt::format("listen(): {}", strerror(errno))};

  spdlog::info("server socket bound fd={}", server_fd);

  accepting.emplace_back(server_fd);
}

void State::event(Accepter &ev) {
  auto fd = ev.socket();
  spdlog::info("accepted fd={}", fd);
  // connection enters reading mode
  reading.emplace_back(fd, 6);

  if (conns.size() < fd)
    conns.resize(fd);

  // remove existing accept
  std::remove(accepting.begin(), accepting.end(), ev);

  // accept another connection
  accepting.emplace_back(server_fd);
}

void State::event(Reader &ev) {
  int fd = ev.fd();

  auto view = ev.data();
  bool empty = view.empty();

  spdlog::info("read {} bytes on fd={}", view.size(), fd);
  // if we read nothing we should close
  if (empty) {
    closing.emplace_back(fd);
    return;
  }

  size_t read_amount{};

  if (auto *conn = get_conn_by_fd(fd))
    read_amount = conn->feed(view);

  std::remove(reading.begin(), reading.end(), ev);
  if (read_amount)
    reading.emplace_back(fd, read_amount);
  else
    closing.emplace_back(fd);
}

void State::event(Closer &ev) {
  int fd = ev.fd();
  spdlog::info("close fd={}", fd);
  if (fd == server_fd)
    end();
  else if (auto *conn = get_conn_by_fd(fd)) {
    spdlog::info("clear connection fd={}", fd);
    conn->clear();
  }
  std::remove(closing.begin(), closing.end(), ev);
}

ConnectionState *State::get_conn_by_fd(int fd) {
  if (fd > conns.size())
    return nullptr;
  return &conns[fd - 1];
}

void State::close_server_socket() {
  if (server_fd == -1)
    return;

  for (const auto &ev : reading) {
    spdlog::info("close client socket fd={}", ev.fd());
    closing.emplace_back(ev.fd());
  }
  closing.emplace_back(server_fd);
  spdlog::info("Close server socket fd={}", server_fd);
}

void State::end() {
  enabled = false;
  io_uring_queue_exit(&g_ring);
  spdlog::info("end()");
}

} // namespace yggdrasil
