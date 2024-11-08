#pragma once

#include <cstdint>
#include <string_view>
#include <sys/uio.h>

#include "format.hpp"

namespace yggdrasil {
/// read only byte view
using byte_view_t = std::basic_string_view<uint8_t>;

struct buffer_printer {
  byte_view_t _buf;
  explicit buffer_printer(byte_view_t buf) : _buf{buf} {};

  std::string str() const;
};

template <> constexpr bool is_formatable<buffer_printer> = true;
} // namespace yggdrasil
