#pragma once

#include "event_base.hpp"

namespace yggdrasil {
void run_loop(State &);

/// @brief closese the server socket and ends the event loop.
struct CloseLoop : public EventBase {
  explicit CloseLoop(State &);
  void completion(State &state, int result) override;
  ~CloseLoop() override;
};
} // namespace yggdrasil
