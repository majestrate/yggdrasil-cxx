#pragma once
#include "buffer_ops.hpp"
#include <array>

namespace yggdrasil {
class ConnectionState {
  enum class ReadState {
    cmd,
    body,
  };

  ReadState _st{ReadState::cmd};
  uint16_t _readsz{};
  std::string _cmd;

public:
  ConnectionState() = default;
  void clear();
  size_t feed(byte_view_t data);
};
} // namespace yggdrasil
