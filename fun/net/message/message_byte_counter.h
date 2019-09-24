#pragma once

#include "fun/net/message/i_message_out.h"
#include "fun/net/message/message.h"
#include "fun/net/message/message_format_config.h"

namespace fun {
namespace net {

class FUN_NET_API MessageByteCounter : public IMessageOut {
 public:
  MessageByteCounter()
      : written_length_(0),
        message_max_length_(MessageFormatConfig::MessageMaxLength) {}

  // IMessageOut interface
 public:
  bool CountingOnly() const override { return true; }

  const uint8* ConstData() const override { return nullptr; }

  uint8* MutableData() override { return nullptr; }

  int32 GetCapacity() const override { return int32_MAX - 1; }

  void SetCapacity(int32 new_capacity) override {
    // do nothing
  }

  int32 MessageMaxLength() const override { return message_max_length_; }

  void SetMessageMaxLength(int32 maximum_length) override {
    message_max_length_ = maximum_length;
  }

  int32 Length() const override { return written_length_; }

  void SetLength(int32 new_length) override { written_length_ = new_length; }

  void Trim() override {
    // do nothing
  }

  bool IsTrimmed() const override { return true; }

  void AddWrittenBytes(int32 Length) override { written_length_ += Length; }

  void RemoveRange(int32 index, int32 length_to_remove) override {
    written_length_ -= length_to_remove;
  }

  void WriteRawBytes(const void* Data, int32 Length) override {
    written_length_ += Length;
  }

  void WriteFixed8(uint8 Value) override { written_length_ += 1; }

  void WriteFixed16(uint16 Value) override { written_length_ += 2; }

  void WriteFixed32(uint32 Value) override { written_length_ += 4; }

  void WriteFixed64(uint64 Value) override { written_length_ += 8; }

  void WriteVarint8(uint8 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint8(Value);
  }

  void WriteVarint16(uint16 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint16(Value);
  }

  void WriteVarint32(uint32 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint32(Value);
  }

  void WriteVarint64(uint64 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint64(Value);
  }

  void WriteVarint8SignExtended(int8 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint8SignExtended(Value);
  }

  void WriteVarint16SignExtended(int16 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint16SignExtended(Value);
  }

  void WriteVarint32SignExtended(int32 Value) override {
    written_length_ += MessageFormat::GetByteLength_Varint32SignExtended(Value);
  }

 private:
  int32 written_length_;
  int32 message_max_length_;
};

}  // namespace net
}  // namespace fun
