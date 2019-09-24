#pragma once

namespace fun {
namespace mongodb {

/**
 * Represents the message header which is always prepended to a
 * MongoDB request or response message.
 */
class FUN_MONGODB_API MessageHeader {
 public:
  static const int32 MSG_HEADER_SIZE = 16;

  enum OpCode {
    OP_REPLY = 1,
    OP_MSG = 1000,
    OP_UPDATE = 2001,
    OP_INSERT = 2002,
    OP_QUERY = 2004,
    OP_GET_MORE = 2005,
    OP_DELETE = 2006,
    OP_KILL_CURSORS = 2007
  };

  explicit MessageHeader(OpCode opcode)
      : opcode_(opcode), message_length_(0), request_id_(0), response_to_(0) {}

  // virtual ~MessageHeader() {}

  // struct MsgHeader {
  //  int32   messageLength;  // total message size, including this
  //  int32   requestID;      // identifier for this message
  //  int32   responseTo;     // requestID from the original request
  //                          //   (used in responses from db)
  //  int32   opCode;         // request type - see table below for details
  //}

  void Read(MessageIn& reader) {
    // TODO 오류처리
    LiteFormat::Read(reader, message_length_);
    LiteFormat::Read(reader, request_id_);
    LiteFormat::Read(reader, response_to_);
    int32 tmp;
    LiteFormat::Read(reader, tmp);
    opcode_ = (Opcode)tmp;
  }

  void Write(MessageOut& wirter) {
    LiteFormat::Write(wirter, message_length_);
    LiteFormat::Write(wirter, request_id_);
    LiteFormat::Write(wirter, response_to_);
    LiteFormat::Write(wirter, (int32)opcode_);
  }

  int32 GetMessageLength() const { return message_length_; }

  OpCode GetOpcode() const { return opcode_; }

  int32 GetRequestId() const { return request_id_; }

  void SetRequestId(int32 id) { request_id_ = id; }

  int32 GetResponseTo() const { return response_to_; }

 private:
  void SetMessageLength(int32 len) {
    fun_check(len >= 0);
    message_length_ = MSG_HEADER_SIZE + len;
  }

  int32 message_length_;
  int32 request_id_;
  int32 response_to_;
  Opcode opcode_;

  friend class Message;
};

}  // namespace mongodb
}  // namespace fun
