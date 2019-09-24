#pragma once

#include "fun/redis/builders/ibuilder.h"

namespace fun {
namespace redis {

class ReplyBuilder {
 public:
  ReplyBuilder();
  virtual ~ReplyBuilder() = default;

  ReplyBuilder(const ReplyBuilder&) = delete;
  ReplyBuilder& operator = (const ReplyBuilder&) = delete;

  ReplyBuilder& operator << (const String& data);

  void operator >> (Reply& reply);
  const Reply& GetFront() const;
  void PopFront();

  bool ReplyAvailable() const;

 private:
  bool BuildReply();

  String buffer_;
  UniquePtr<IBuilder> builder_;
  TArray<Reply> available_replies_;
};

} // namespace redis
} // namespace fun
