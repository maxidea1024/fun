#include "fun/redis/builders/simple_string_builder.h"

namespace fun {
namespace redis {

SimpleStringBuilder::SimpleStringBuilder() : string_(), reply_ready_(false) {}

IBuilder& SimpleStringBuilder::operator<<(String& buffer) {
  if (reply_ready_) {
    return *this;
  }

  const int32 crlf_position = buffer.IndexOf("\r\n");
  if (crlf_position == INVALID_INDEX) {
    return *this;
  }

  string_ = buffer.Mid(0, crlf_position);
  reply_.Set(string_, Reply::StringType::SimpleString);
  buffer.Remove(0, crlf_position + 2);  // remove taken (also cr and lf)
  reply_ready_ = true;
  return *this;
}

bool SimpleStringBuilder::ReplyReady() const { return reply_ready_; }

Reply SimpleStringBuilder::GetReply() const { return reply_; }

const String& SimpleStringBuilder::GetSimpleString() const { return string_; }

}  // namespace redis
}  // namespace fun
