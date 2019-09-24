#pragma once

#include "fun/redis/reply.h"

namespace fun {
namespace redis {

class IBuilder {
 public:
  virtual ~IBuilder() = default;

  virtual IBuilder& operator << (String& buffer) = 0;

  virtual bool ReplyReady() const = 0;

  //TODO 참조로 반환해도 되지 않을런지??
  virtual Reply GetReply() const = 0;
};

} // namespace redis
} // namespace fun
