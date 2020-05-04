#pragma once

#include "fun/redis/builders/ibuilder.h"
#include "fun/redis/builders/integer_builder.h"

namespace fun {
namespace redis {

class ArrayBuilder : public IBuilder {
 public:
  ArrayBuilder();
  virtual ~ArrayBuilder() = default;

  ArrayBuilder(const ArrayBuilder&) = delete;
  ArrayBuilder& operator=(const ArrayBuilder&) = delete;

  IBuilder& operator<<(String& buffer) override;
  bool ReplyReady() const override;
  Reply GetReply() const override;

 private:
  bool FetchArraySize(String& buffer);
  bool BuildRow(String& buffer);

  IntegerBuilder integer_builder_;
  uint64 array_size_;
  UniquePtr<IBuilder> current_builder_;
  bool reply_ready_;
  Reply reply_;
};

}  // namespace redis
}  // namespace fun
