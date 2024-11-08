#pragma once
#include <fmt/format.h>

namespace yggdrasil {

template <typename T> constexpr bool is_formatable = false;

}

namespace fmt {
template <typename T>
struct formatter<T, char, std::enable_if_t<yggdrasil::is_formatable<T>>>
    : formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const T &val, FormatContext &ctx) const {
    return formatter<std::string_view>::format(val.str(), ctx);
  }
};

} // namespace fmt
