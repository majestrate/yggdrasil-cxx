#include <spdlog/spdlog.h>
#include <yggdrasil/connection.hpp>
#include <yggdrasil/endian.hpp>
#include <yggdrasil/varint.hpp>

namespace yggdrasil {
size_t ConnectionState::feed(byte_view_t data) {
  if (data.empty())
    return _readsz;
  spdlog::info("state={}", std::to_underlying(_st));
  spdlog::info("buffer contents: {}", buffer_printer{data});

  if (_st == ReadState::cmd) {
    if (data.size() < 6)
      return 0;
    _cmd.resize(4);
    std::copy_n(data.begin(), 4, _cmd.data());
    spdlog::info("cmd='{}'", _cmd);
    _readsz = native16_from_big(data.begin() + 4);
    _st = ReadState::body;
    auto itr = data.begin() + 6;
    spdlog::info("read frame of size {}", int{_readsz});
    return feed(byte_view_t{itr, data.end()});
  } else if (_st == ReadState::body) {
  }
  return 0;
}

void ConnectionState::clear() {
  _st = ReadState::cmd;
  _readsz = 0;
}
} // namespace yggdrasil
