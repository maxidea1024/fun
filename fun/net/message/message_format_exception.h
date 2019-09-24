#pragma once

#include "fun/net/message/message.h"
#include "fun/base/exception.h"

namespace fun {
namespace net {

class MessageFormatException : public Exception {
 public:
  MessageFormatException() : Exception() {}
  MessageFormatException(const String& msg) : Exception(msg) {}

  FUN_NET_API static MessageFormatException Misuse(const String& message);
  FUN_NET_API static MessageFormatException MoreDataAvailable();
  FUN_NET_API static MessageFormatException TruncatedMessage();
  FUN_NET_API static MessageFormatException NegativeSize();
  FUN_NET_API static MessageFormatException MalformedVarint();
  FUN_NET_API static MessageFormatException InvalidTag();
  FUN_NET_API static MessageFormatException InvalidFieldId(int32 field_id);
  FUN_NET_API static MessageFormatException MessageInLengthLimited(int32 length, int32 limit);
  FUN_NET_API static MessageFormatException MessageOutLengthLimited(int32 length, int32 limit);
  FUN_NET_API static MessageFormatException RecursionLimitExceeded();
  FUN_NET_API static MessageFormatException UnderflowRecursionDepth();
  FUN_NET_API static MessageFormatException RequiredFieldIsMissing(const String& field_name, const String& struct_name);
};

} // namespace net
} // namespace fun
