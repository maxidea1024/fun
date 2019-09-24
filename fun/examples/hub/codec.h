#pragma once

#include "fun/base/types.h"
#include "fun/net/buffer.h"

#include <boost/noncopyable.hpp>

namespace pubsub {

enum ParseResult {
  kError,
  kSuccess,
  kContinue,
};

ParseResult ParseMessage(Buffer* buf,
                         String* cmd,
                         String* topic,
                         String* content);

} // namespace pubsub
