#pragma once

#define FUN_REDIS_API /* 임시로 선언해놓음 차후에 모듈화 지원하면, 정상으로 \
                         돌려놓아여함. */

#include "fun/redis/error.h"
#include "fun/redis/reply.h"

namespace fun {

// namespace redis {
FUN_REDIS_API DECLARE_LOG_CATEGORY_EXTERN(LogRedis, Info, All);
//}

}  // namespace fun
