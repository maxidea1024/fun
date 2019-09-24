#pragma once

#include "fun/net/message/message.h"

namespace fun {
namespace net {

class MessageFormatConfig {
 public:
  FUN_NET_API static int32 MessageMinLength;
  FUN_NET_API static int32 MessageMaxLength;
  FUN_NET_API static int32 MaxContainerElementCount;
  FUN_NET_API static int32 DefaultRecursionLimit;
};

} // namespace net
} // namespace fun
