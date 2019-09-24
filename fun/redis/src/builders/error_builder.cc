#include "slky/redis/builders/error_builder.h"

namespace fun {
namespace redis {

ErrorBuilder::ErrorBuilder()
{
}

IBuilder& ErrorBuilder::operator << (String& buffer)
{
  simple_string_builder_ << buffer;

  if (simple_string_builder_.ReplyReady()) {
    reply_.Set(simple_string_builder_.GetSimpleString(), Reply::StringType::Error);
  }

  return *this;
}

bool ErrorBuilder::ReplyReady() const
{
  return simple_string_builder_.ReplyReady();
}

Reply ErrorBuilder::GetReply() const
{
  return reply_;
}

const String& ErrorBuilder::GetError() const
{
  return simple_string_builder_.GetSimpleString();
}

} // namespace redis
} // namespace fun
