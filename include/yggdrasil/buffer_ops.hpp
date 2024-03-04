#pragma once

#include <cstdint>
#include <string_view>
#include <sys/uio.h>

namespace yggdrasil {
/// read only byte view
using byte_view_t = std::basic_string_view<uint8_t>;

} // namespace yggdrasil