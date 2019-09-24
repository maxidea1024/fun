#pragma once

#include "fun/mongodb/request.h"
#include "fun/mongodb/document.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class InsertRequest> InsertRequestPtr;

/**
 * This message is used to insert one or more documents into a collection.
 */
class FUN_MONGODB_API InsertRequest : public Request {
 public:
  enum Flags {
    INSERT_DEFAULT = 0,
    INSERT_CONTINUE_ON_ERROR = 1,
  };

  InsertRequest(const String& collection_name, Flags flags = INSERT_DEFAULT)
    : Request(MessageHeader::OP_INSERT),
      flags_(flags),
      full_collection_name_(collection_name) {}

  //virtual ~InsertRequest() {}

  /**
   * Adds a new document for insertion. A reference to the empty document is
   * returned. InsertRequest is the owner of the Document and will free it
   * on destruction.
   */
  Document& AddNewDocument() {
    DocumentPtr new_doc(new Document());
    documents_.Add(new_doc);
    return **new_doc;
  }

  /**
   * Returns the documents to insert into the database.
   */
  fun::Array<Document>& GetDocuments() {
    return documents_;
  }

 protected:
  void BuildRequest(MessageOut& wirter) override {
    //struct {
    //  MsgHeader header;             // standard message header
    //  int32     flags;              // bit vector - see below
    //  cstring   fullCollectionName; // "dbname.collectionname"
    //  document* documents;          // one or more documents to insert into the collection
    //}
    fun_check(!documents_.IsEmpty());

    LiteFormat::Write(wirter, (int32)flags_);

    BsonWriter BsonWriter(writer_);
    BsonWriter.WriteCString(full_collection_name_);

    for (const auto& document : documents_) {
      BsonWriter.Write(document);
    }
  }

 private:
  Flags flags_;
  String full_collection_name_;
  fun::Array<Document> documents_;
};

} // namespace mongodb
} // namespace fun
