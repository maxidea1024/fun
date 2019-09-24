#include "fun/redis/builders/integer_builder.h"

namespace fun {
namespace redis {

IntegerBuilder::IntegerBuilder()
  : number_(0)
  , negative_multiplier_(1)
  , reply_ready_(false)
{
}

IBuilder& IntegerBuilder::operator << (String& buffer)
{
  if (reply_ready_) {
    return *this;
  }

  const int32 crlf_position = buffer.IndexOf("\r\n");
  if (crlf_position == INVALID_INDEX) {
    return *this;
  }

  for (int32 i = 0; i < crlf_position; ++i) {
    if (i == 0 && negative_multiplier_ == 1 && buffer[i] == '-') {
      negative_multiplier_ = -1;
      continue;
    }
    else if (!CharTraitsA::IsDigit(buffer[i])) {
      //TODO throw exception
      //"Invalid character for integer redis reply"
    }

    number_ *= 10;
    number_ += buffer[i] - '0';
  }

  buffer.Remove(0, crlf_position+2);
  reply_.Set(number_ * negative_multiplier_);
  reply_ready_ = true;

  return *this;
}

bool IntegerBuilder::ReplyReady() const
{
  return reply_ready_;
}

Reply IntegerBuilder::GetReply() const
{
  return reply_;
}

int64 IntegerBuilder::GetInteger() const
{
  return number_ * negative_multiplier_;
}

} // namespace redis
} // namespace fun
