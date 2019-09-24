#pragma once

#include "fun/redis/builders/integer_builder.h"

namespace fun {
namespace redis {

class BulkStringBuilder : public IBuilder {
 public:
  BulkStringBuilder();
  virtual ~BulkStringBuilder() = default;

  BulkStringBuilder(const BulkStringBuilder&) = delete;
  BulkStringBuilder& operator = (const BulkStringBuilder&) = delete;

  IBuilder& operator << (String& buffer) override;
  bool ReplyReady() const override;
  Reply GetReply() const override;

  const String& GetBulkString() const;
  bool IsNull() const;

 private:
  void BuildReply();
  bool FetchSize(String& buffer);
  void FetchString(String& buffer);

  IntegerBuilder integer_builder_;
  int32 string_size_;
  String string_;
  bool is_null_;
  bool reply_ready_;
  Reply reply_;
};

} // namespace redis
} // namespace fun
