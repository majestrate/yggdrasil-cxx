#pragma once
#include <event2/listener.h>
#include <memory>

using EventLoopDeleter_t = decltype(&::event_base_free);
using EventLoop_ptr = std::unique_ptr<::event_base, EventLoopDeleter_t>;
