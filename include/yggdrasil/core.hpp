#pragma once

#include "connection.hpp"
#include "event_base.hpp"
#include <deque>
#include <memory>

namespace yggdrasil {

struct Resources;

std::shared_ptr<Resources> make_resources();

/// @brief the god object holding all statefulness.
class State {

  void end();
  const Resources &_res;
  std::pmr::vector<ConnectionState> conns;

  ConnectionState *get_conn_by_fd(int fd);

public:
  std::pmr::deque<Accepter> accepting;
  std::pmr::deque<Reader> reading;
  std::pmr::deque<Closer> closing;

  int server_fd{-1};
  bool enabled{true};

  explicit State(Resources &res);
  State(const State &) = delete;
  State(State &&) = delete;

  void bind_server_socket(const SockAddr &saddr);
  void close_server_socket();

  void event(Accepter &ev);
  void event(Reader &ev);
  void event(Closer &ev);
};

} // namespace yggdrasil
