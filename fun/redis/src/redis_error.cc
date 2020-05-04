#include "fun/redis/error.h"

namespace fun {
// namespace redis {

// TODO Redis.cpp에 옮겨주자..
DEFINE_LOG_CATEGORY(LogRedis);

FUN_IMPLEMENT_EXCEPTION(RedisException, "Redis Exception");

//} // namespace redis
}  // namespace fun
