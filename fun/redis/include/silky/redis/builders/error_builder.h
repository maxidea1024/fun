#pragma once

#include "fun/redis/builders/ibuilder.h"
#include "fun/redis/builders/simple_string_builder.h"

namespace fun {
namespace redis {

class ErrorBuilder : public IBuilder {
 public:
  ErrorBuilder();
  virtual ~ErrorBuilder() = default;

  ErrorBuilder(const ErrorBuilder&) = delete;
  ErrorBuilder& operator = (const ErrorBuilder&) = delete;

  IBuilder& operator << (String& buffer) override;
  bool ReplyReady() const override;
  Reply GetReply() const override;

  const String& GetError() const;

 private:
  SimpleStringBuilder simple_string_builder_;
  Reply reply_;
};

} // namespace redis
} // namespace fun
