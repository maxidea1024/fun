#pragma once

#include "fun/mongodb/document.h"
#include "fun/mongodb/request.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class DeleteRequest> DeleteRequestPtr;

/**
 * This message is used to remove one or more documents from a collection.
 */
class FUN_MONGODB_API DeleteRequest : public Request {
 public:
  enum Flags {
    DELETE_DEFAULT = 0,
    DELETE_SINGLE_REMOVE = 1,
  };

 public:
  DeleteRequest(const String& collection_name, Flags flags = DELETE_DEFAULT)
      : Request(MessageHeader::OP_DELETE),
        flags_(flags),
        full_collection_name_(collection_name) {}

  DeleteRequest(const String& collection_name, bool just_one)
      : Request(MessageHeader::OP_DELETE),
        flags_(just_one ? DELETE_SINGLE_REMOVE : DELETE_DEFAULT),
        full_collection_name_(collection_name) {}

  // virtual ~DeleteRequest() {}

  Flags GetFlags() const { return flags_; }

  void SetFlags(Flags flags) { flags_ = flags; }

  Document& GetSelector() { return selector_; }

 protected:
  void BuildRequest(MessageOut& wirter) {
    // struct {
    //  MsgHeader header;             // standard message header
    //  int32     ZERO;               // 0 - reserved for future use
    //  cstring   fullCollectionName; // "dbname.collectionname"
    //  int32     flags;              // bit vector - see below for details.
    //  document  selector;           // query object.  See below for details.
    //}
    LiteFormat::Write(wirter, (int32)0);  // 0 - reserved for future use
    BsonWriter(wirter).WriteCString(full_collection_name_));
    LiteFormat::Write(wirter, (int32)flags_));
    selector_.Write(wirter);
  }

 private:
  Flags flags_;
  String full_collection_name_;
  Document selector_;
};

}  // namespace mongodb
}  // namespace fun
