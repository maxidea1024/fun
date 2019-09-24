#include "fun/redis/builders/reply_builder.h"
#include "fun/redis/builders/builder_factory.h"

namespace fun {
namespace redis {

ReplyBuilder::ReplyBuilder()
  : builder_()
{
}

ReplyBuilder& ReplyBuilder::operator << (const String& data)
{
  buffer_ += data;
  while (BuildReply());
  return *this;
}

void ReplyBuilder::operator >> (Reply& reply)
{
  reply = GetFront();
}

const Reply& ReplyBuilder::GetFront() const
{
  if (!ReplyAvailable()) {
    //TODO throw exception
    //"No available reply"
  }
  return available_replies_[0];
}

void ReplyBuilder::PopFront()
{
  if (!ReplyAvailable()) {
    //TODO throw exception
    //"No available reply"
  }
  available_replies_.RemoveAt(0);
}

bool ReplyBuilder::ReplyAvailable() const
{
  return available_replies_.Count() > 0;
}

bool ReplyBuilder::BuildReply()
{
  if (buffer_.IsEmpty()) {
    return false;
  }

  if (!builder_) {
    builder_ = Builders::CreateBuilder(buffer_.First());
    buffer_.RemoveFirst();
  }

  *builder_ << buffer_;

  if (builder_->ReplyReady()) {
    available_replies_.Add(builder_->GetReply());
    builder_ = nullptr;
  }

  return true;
}

} // namespace redis
} // namespace fun
