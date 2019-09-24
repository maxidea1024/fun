#pragma once

#define FUN_ZOOKEEPER_API  /* TEMP */

#include "fun/zk/zk_client.h"

namespace fun {

FUN_ZOOKEEPER_API FUN_DECLARE_LOG_CATEGORY_EXTERN(LogZookeeper, Info, All);
FUN_DECLARE_EXCEPTION(FUN_ZOOKEEPER_API, ZookeeperException, Exception);

} // namespace fun
