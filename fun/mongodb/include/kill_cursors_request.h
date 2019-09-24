//TODO 이 메시지는 내부적으로만 사용되므로, private에 위치시킴.
#pragma once

#include "fun/mongodb/request.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class KillCursorRequest> KillCursorRequestPtr;

/**
 * This message is used to close an active cursor in the database.
 */
class FUN_MONGODB_API KillCursorRequest : public Request {
 public:
  KillCursorRequest() : Request(MessageHeader::OP_KILL_CURSORS) {}

  //virtual ~KillCursorRequest() {}

  TArray<int64>& GetCursors() {
    return cursors_;
  }

 protected:
  void BuildRequest(MessageOut& wirter) {
    //struct {
    //  MsgHeader header;            // standard message header
    //  int32     ZERO;              // 0 - reserved for future use
    //  int32     numberOfCursorIDs; // number of cursorIDs in message
    //  int64*    cursorIDs;         // sequence of cursorIDs to close
    //}

    LiteFormat::Write(wirter, (int32)0); // 0 - reserved for future use
    LiteFormat::Write(wirter, (int32)cursors_.Count());
    for (const auto& cursor : cursors_) {
      LiteFormat::Write(wirter, cursor);
    }
  }

 private:
  fun::Array<int64> cursors_;
};

} // namespace mongodb
} // namespace fun
