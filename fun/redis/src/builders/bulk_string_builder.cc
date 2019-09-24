#include "fun/redis/builders/bulk_string_builder.h"

namespace fun {
namespace redis {

BulkStringBuilder::BulkStringBuilder()
  : string_size_(0)
  , string_()
  , is_null_(false)
  , reply_ready_(false)
{
}

IBuilder& BulkStringBuilder::operator << (String& buffer)
{
  if (reply_ready_) {
    return *this;
  }

  if (!FetchSize(buffer) || reply_ready_) {
    return *this;
  }

  FetchString(buffer);
  return *this;
}

bool BulkStringBuilder::ReplyReady() const
{
  return reply_ready_;
}

Reply BulkStringBuilder::GetReply() const
{
  return reply_;
}

const String& BulkStringBuilder::GetBulkString() const
{
  return string_;
}

bool BulkStringBuilder::IsNull() const
{
  return is_null_;
}

void BulkStringBuilder::BuildReply()
{
  if (is_null_) {
    reply_.Set();
  }
  else {
    reply_.Set(string_, Reply::StringType::BulkString);
  }

  reply_ready_ = true;
}

bool BulkStringBuilder::FetchSize(String& buffer)
{
  if (integer_builder_.ReplyReady()) {
    return true;
  }

  integer_builder_ << buffer;
  if (!integer_builder_.ReplyReady()) {
    return false;
  }

  string_size_ = integer_builder_.GetInteger();
  if (string_size_ == -1) {
    is_null_ = true;
    BuildReply();
  }

  return true;
}

void BulkStringBuilder::FetchString(String& buffer)
{
  if (buffer.Len() < string_size_+2) { // 아직 부족함...
    return;
  }

  if (buffer[string_size_] != '\r' || buffer[string_size_+1] != '\n') {
    //invalid sequence.
    //TODO throw exception "wrong ending sequence"
    return;
  }

  string_ = buffer.Mid(0, string_size_);
  buffer.Remove(0, string_size_ + 2);
  BuildReply();
}

} // namespace redis
} // namespace fun
