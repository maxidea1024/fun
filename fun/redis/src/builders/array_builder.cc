#include "fun/redis/builders/array_builder.h"
#include "fun/redis/builders/builder_factory.h"

namespace fun {
namespace redis {

ArrayBuilder::ArrayBuilder()
    : current_builder_(), reply_ready_(false), reply_() {}

IBuilder& ArrayBuilder::operator<<(String& buffer) {
  if (reply_ready_) {
    return *this;
  }

  if (!FetchArraySize(buffer)) {
    return *this;
  }

  while (buffer.Len() && !reply_ready_) {
    if (!BuildRow(buffer)) {
      return *this;
    }
  }

  return *this;
}

bool ArrayBuilder::ReplyReady() const { return reply_ready_; }

Reply ArrayBuilder::GetReply() const { return reply_; }

bool ArrayBuilder::FetchArraySize(String& buffer) {
  if (integer_builder_.ReplyReady()) {
    return true;
  }

  integer_builder_ << buffer;
  if (!integer_builder_.ReplyReady()) {
    return false;
  }

  const int64 size = integer_builder_.GetInteger();
  if (size < 0) {
    reply_.Set();
    reply_ready_ = true;
  } else if (size == 0) {
    reply_ready_ = true;
  }

  array_size_ = size;
  return true;
}

bool ArrayBuilder::BuildRow(String& buffer) {
  if (!current_builder_) {
    current_builder_ = Builders::CreateBuilder(buffer.First());
    buffer.RemoveFirst();
  }

  *current_builder_ << buffer;
  if (!current_builder_->ReplyReady()) {
    return false;
  }

  reply_ << current_builder_->GetReply();
  current_builder_ = nullptr;

  if (reply_.AsArray().Count() == array_size_) {
    reply_ready_ = true;
  }

  return true;
}

}  // namespace redis
}  // namespace fun
