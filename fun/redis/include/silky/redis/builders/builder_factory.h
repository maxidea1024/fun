#pragma once

#include "fun/redis/builders/ibuilder.h"

namespace fun {
namespace redis {

class FUN_REDIS_API Builders {
 public:
  static UniquePtr<IBuilder> CreateBuilder(char resp_tag);
};

} // namespace redis
} // namespace fun
