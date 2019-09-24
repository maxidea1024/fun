#pragma once

#include "fun/mongodb/document.h"
#include "fun/mongodb/request.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class UpdateRequest> UpdateRequestPtr;

/**
 * This message is used to update a document in a collection.
 */
class FUN_MONGODB_API UpdateRequest : public Request {
 public:
  // TODO Flags<T>로 변경하도록 하자.
  enum Flags {
    UPDATE_DEFAULT = 0,
    UPDATE_UPSERT = 1,
    UPDATE_MULTIUPDATE = 2,
  };

  UpdateRequest(const String& collection_name, Flags flags = UPDATE_DEFAULT)
      : Request(MessageHeader::OP_UPDATE),
        flags(flags_),
        full_collection_name_(CollectionName_),
        selector_(),
        update_() {}

  // virtual ~UpdateRequest() {}

  Flags GetFlags() const { return flags_; }

  void SetFlags(Flags flags) { flags_ = flags; }

  Document& GetSelector() { return selector_; }

  Document& GetUpdate() { return update_; }

 protected:
  void BuildRequest(MessageOut& wirter) {
    // struct OP_UPDATE {
    //  MsgHeader header;             // standard message header
    //  int32     ZERO;               // 0 - reserved for future use
    //  cstring   fullCollectionName; // "dbname.collectionname"
    //  int32     flags;              // bit vector. see below
    //  document  selector;           // the query to select the document
    //  document  update;             // specification of the update to perform
    //}
    LiteFormat::Write(wirter, (int32)0);  // 0 - reserved for future use
    BsonWriter BsonWriter(writer_);
    BsonWriter.WriteCString(full_collection_name_);
    LiteFormat::Write(wirter, (int32)flags_);
    selector_.Write(writer_);
    update_.Write(writer_);
  }

 private:
  Flags flags_;
  String full_collection_name_;
  Document selector_;
  Document update_;
};

}  // namespace mongodb
}  // namespace fun
