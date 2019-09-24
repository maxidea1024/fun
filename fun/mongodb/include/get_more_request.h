//TODO 내부에서만 사용되므로, private에 위치키시킴.
#pragma once

#include "fun/mongodb/request.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<GetMoreRequest> GetMoreRequestPtr;

/**
 * A GetMoreRequest is used to query the database for more documents in a collection
 * after a query request is send (OP_GETMORE).
 */
class FUN_MONGODB_API GetMoreRequest : public Request {
 public:
  GetMoreRequest(const String& collection_name, int64 cursor_id)
    : Request(MessageHeader::OP_GET_MORE),
      full_collection_name_(collection_name),
      number_to_return_(0),
      cursor_id_(cursor_id) {
  }

  //virtual ~GetMoreRequest() {}

  int32 GetLimit() const {
    return number_to_return_;
  }
  void SetLimit(int32 N) {
    number_to_return_ = N;
  }

  //int32 GetNumberToReturn() const {
  //  return number_to_return_;
  //}
  //
  //void SetNumberToReturn(int32 N) {
  //  number_to_return_ = N;
  //}

  int64 GetCursorId() const {
    return cursor_id_;
  }

 protected:
  void BuildRequest(MessageOut& wirter) {
    //struct OP_GET_MORE
    //{
    //  MessageHeader header;         //message header
    //  int32 ZERO_RESERVED;          //reserved for future use
    //  cstring full_collection_name; // full collection name
    //  int32 number_to_return;
    //  int32 cursor_id;
    //};
    LiteFormat::Write(wirter, (int32)0); // 0 - reserved for future use
    BsonWriter(wirter).WriteCString(full_collection_name_);
    LiteFormat::Write(wirter, number_to_return_);
    LiteFormat::Write(wirter, cursor_id_);
  }

 private:
  String full_collection_name_;
  int32 number_to_return_;
  int64 cursor_id_;
};

} // namespace mongodb
} // namespace fun
