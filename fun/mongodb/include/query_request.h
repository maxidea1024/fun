#pragma once

#include "fun/mongodb/document.h"
#include "fun/mongodb/request.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class QueryRequest> RequestPtr;

/**
 * This message is used to query the database for documents in a collection.
 */
class FUN_MONGODB_API QueryRequest : public Request {
 public:
  enum Flags {
    QUERY_DEFAULT = 0,
    QUERY_TAILABLE_CURSOR = 2,
    QUERY_SLAVE_OK = 4,
    QUERY_NO_CURSOR_TIMEOUT = 16,
    QUERY_AWAIT_DATA = 32,
    QUERY_EXHAUST = 64,
    QUERY_PARTIAL = 128
  };

 public:
  QueryRequest(const String& collection_name, Flags flags = QUERY_DEFAULT)
      : Request(MessageHeader::OP_QUERY),
        flags_(flags),
        full_collection_name_(collection_name),
        number_to_skip_(0),
        number_to_return_(0),
        selector_(),
        return_field_selector_() {}

  // virtual ~QueryRequest() {}

  Flags GetFlags() const { return flags_; }
  void SetFlags(Flags flags) { flags_ = flags; }

  const String& GetFullCollectionName() const { return full_collection_name_; }

  int32 GetSkip() const { return number_to_skip_; }
  void SetSkip(int32 N) { number_to_skip_ = N; }

  int32 GetLimit() const { return number_to_return_; }
  void SetLimit(int32 N) { number_to_return_ = N; }

  Document& GetSelector() { return selector_; }

  Document& GetReturnFieldSelector() { return return_field_selector_; }

 protected:
  void BuildRequest(MessageOut& wirter) {
    // struct OP_QUERY {
    //  MsgHeader header;                 // standard message header
    //  int32     flags;                  // bit vector of query options.  See
    //  below for details. cstring   fullCollectionName ;    //
    //  "dbname.collectionname" int32     numberToSkip;           // number of
    //  documents to skip int32     numberToReturn;         // number of
    //  documents to return
    //                    //  in the first OP_REPLY batch
    //  document  query;                  // query object.  See below for
    //  details. [ document  returnFieldsSelector; ] // Optional. Selector
    //  indicating the fields
    //                    //  to return.  See below for details.
    //}

    LiteFormat::Write(wirter, (int32)flags_);
    BsonWriter(wirter).WriteCString(full_collection_name_);
    LiteFormat::Write(wirter, number_to_skip_);
    LiteFormat::Write(wirter, number_to_return_);
    selector_.Write(wirter);
    if (!return_field_selector_.IsEmpty()) {
      return_field_selector_.Write(wirter);
    }
  }

 private:
  Flags flags_;
  String full_collection_name_;
  int32 number_to_skip_;
  int32 number_to_return_;
  Document selector_;
  Document return_field_selector_;
};

}  // namespace mongodb
}  // namespace fun
