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
};

} // namespace yggdrasil