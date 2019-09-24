#pragma once

#include "fun/mongodb/document.h"
#include "fun/mongodb/message.h"

namespace fun {
namespace mongodb {

typedef SharedPtr<class Response> ResponsePtr;

/**
 * This class represents a response (OP_REPLY) from MongoDB.
 */
class FUN_MONGODB_API Response : public Message {
 public:
  enum Flags {
    INSERT_DEFAULT = 0,
    INSERT_CONTINUE_ON_ERROR = 1,
  };

  Response()
      : Message(MessageHeader::OP_REPLY),
        response_flags_(0),
        cursor_id_(0),
        starting_from_(0),
        number_of_returned_(0) {}

  // virtual ~Response() {}

  int64 GetCursorId() const { return cursor_id_; }

  void Reset() {
    response_flags_ = 0;
    starting_from_ = 0;
    cursor_id_ = 0;
    number_of_returned_ = 0;
    documents_.Clear();
  }

  int32 Count() const { return documents_.Count(); }

  fun::Array<Document>& GetDocuments() { return documents_; }

  explicit operator bool() const { return not documents_.IsEmpty(); }

  bool IsEmpty() const { return documents_.IsEmpty(); }

  bool HasDocuments() const { return not documents_.IsEmpty(); }

  // TODO CMessageIn으로 처리하는게 바람직하지 않을런지??
  void Read(std::istream& is) {
    // struct {
    //  MsgHeader header;         // standard message header
    //  int32     responseFlags;  // bit vector - see details below
    //  int64     cursorID;       // cursor id if client needs to do get more's
    //  int32     startingFrom;   // where in the cursor this reply is starting
    //  int32     numberReturned; // number of documents in the reply
    //  document* documents;      // documents
    //}

    // TODO
    fun_check(0);

    /*

    Header_.Read(Reader);

    LiteFormat::Read(Reader, response_flags_);
    LiteFormat::Read(Reader, cursor_id_);
    LiteFormat::Read(Reader, starting_from_);
    LiteFormat::Read(Reader, NumberOfReturned);

    for (int32 i = 0; i < NumberOfReturned; ++i) {
      DocumentPtr Doc(new Document());
      Doc->Read(Reader);
      documents_.Add(Doc);
    }

    */
  }

 private:
  int32 response_flags_;
  int64 cursor_id_;
  int32 starting_from_;
  int32 number_of_returned_;
  fun::Array<Document> documents_;
};

}  // namespace mongodb
}  // namespace fun
