#pragma once

#include "fun/redis/builders/ibuilder.h"

namespace fun {
namespace redis {

class IntegerBuilder : public IBuilder {
 public:
  IntegerBuilder();
  virtual ~IntegerBuilder() = default;

  IntegerBuilder(const IntegerBuilder&) = delete;
  IntegerBuilder& operator=(const IntegerBuilder&) = delete;

  IBuilder& operator<<(String& buffer) override;
  bool ReplyReady() const override;
  Reply GetReply() const override;

  int64 GetInteger() const;

 private:
  int64 number_;
  int64 negative_multiplier_;
  bool reply_ready_;
  Reply reply_;
};

}  // namespace redis
}  // namespace fun
