#pragma once

#include "fun/mongodb/message.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class Request> RequestPtr;

/**
 * The base class of request message.
 */
class FUN_MONGODB_API Request : public Message {
 public:
  explicit Request(MessageHeader::OpCode opcode) : Message(opcode) {}
  // virtual ~Request() {}

  void WireSend(std::ostream& os) {
    // TODO
    // struct MsgHeader {
    //  int32   messageLength; // total message size, including this
    //  int32   requestID;     // identifier for this message
    //  int32   responseTo;    // requestID from the original request
    //               //   (used in responses from db)
    //  int32   opCode;        // request type - see table below for details
    //}

    /*
    //임시 버퍼를 사용해 기록한 후 헤더를 채우고, 뒤에 붙이는 방식 최적화가
    필요해보임. BuildRequest(wirter);

    SetMessageLength(wirter.Length());

    Header_.Write(Write);

    WritePayload(wirter);
    */
  }

 protected:
  virtual void BuildRequest(MessageOut& wirter) = 0;
};

}  // namespace mongodb
}  // namespace fun
