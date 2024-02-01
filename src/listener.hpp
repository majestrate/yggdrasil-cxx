#pragma once
#include "events.hpp"

namespace yggdrasil {

class SockAddr;

class Listener {
  struct EvConnListenerDeleter {
    void operator()(::evconnlistener *ptr) const {
      if (ptr)
        ::evconnlistener_free(ptr);
    }
  };

  using EvConnListener_ptr =
      std::unique_ptr<::evconnlistener, EvConnListenerDeleter>;

  static void on_inbound_connection(evconnlistener *listener,
                                    evutil_socket_t fd, sockaddr *saddr,
                                    int sockelen, void *udata);

  EvConnListener_ptr _ptr;

public:
  Listener(const EventLoop_ptr &loop, const SockAddr &saddr);
};
} // namespace yggdrasil
