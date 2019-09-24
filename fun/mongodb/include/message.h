#pragma once

#include "fun/mongodb/message_header.h"

namespace fun {
namespace mongodb {

/**
 * Base class for all messages send or retrieved from MongoDB server.
 */
class FUN_MONGODB_API Message {
 public:
  explicit Message(MessageHeader::Opcode opcode)
    : Message(opcode) {}

  virtual ~Message() {}

  MessageHeader& GetHeader() {
    return header_;
  }

 protected:
  /** Message header. */
  MessageHeader header_;

  void SetMessageLength(int32 len) {
    fun_check(len > 0);
    header_.SetMessageLength(len);
  }
};

} // namespace mongodb
} // namespace fun
