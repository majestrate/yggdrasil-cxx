#pragma once

#include "event_base.hpp"
#include <deque>
#include <memory>

namespace yggdrasil {

struct Resources;

std::shared_ptr<Resources> make_resources();

/// @brief the god object holding all statefulness.
class State {
  Resources &_res;

  void end();

public:
  std::pmr::deque<Accepter> accepting;
  std::pmr::deque<Reader> reading;
  std::pmr::deque<Closer> closing;

  int server_fd;
  bool enabled{true};

  explicit State(Resources &res);
  State(const State &) = delete;
  State(State &&) = delete;

  void bind_server_socket(const SockAddr &saddr);
  void close_server_socket();

  void recv_msg(Accepter *ev, int fd);
  void recv_msg(Reader *ev, ssize_t num);
  void recv_msg(Closer *ev);
};

} // namespace yggdrasil