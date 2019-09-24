#pragma once

#include "fun/redis/builders/ibuilder.h"

namespace fun {
namespace redis {

class SimpleStringBuilder : public IBuilder {
 public:
  SimpleStringBuilder();
  virtual ~SimpleStringBuilder() = default;

  SimpleStringBuilder(const SimpleStringBuilder&) = delete;
  SimpleStringBuilder& operator = (const SimpleStringBuilder&) = delete;

  IBuilder& operator << (String& buffer) override;
  bool ReplyReady() const override;
  Reply GetReply() const override;

  const String& GetSimpleString() const;

 private:
  String string_;
  bool reply_ready_;
  Reply reply_;
};

} // namespace redis
} // namespace fun
