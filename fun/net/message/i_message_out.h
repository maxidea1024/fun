#pragma once

#include "fun/net/message/message.h"

namespace fun {
namespace net {

class FUN_NET_API IMessageOut {
 public:
  virtual ~IMessageOut() {}

  virtual bool CountingOnly() const { return false; }

  virtual const uint8* ConstData() const = 0;
  virtual uint8* MutableData() = 0;

  virtual int32 GetCapacity() const = 0;
  virtual void SetCapacity(int32 new_capacity) = 0;

  virtual int32 MessageMaxLength() const = 0;
  virtual void SetMessageMaxLength(int32 maximum_length) = 0;

  virtual int32 Length() const = 0;
  virtual void SetLength(int32 new_length) = 0;

  virtual void Trim() = 0;
  virtual bool IsTrimmed() const = 0;

  virtual void AddWrittenBytes(int32 length) = 0;

  virtual void RemoveRange(int32 index, int32 length_to_remove) = 0;
  virtual void WriteRawBytes(const void* data, int32 length) = 0;

  virtual void WriteFixed8(uint8 value) = 0;
  virtual void WriteFixed16(uint16 value) = 0;
  virtual void WriteFixed32(uint32 value) = 0;
  virtual void WriteFixed64(uint64 value) = 0;

  virtual void WriteVarint8(uint8 value) = 0;
  virtual void WriteVarint16(uint16 value) = 0;
  virtual void WriteVarint32(uint32 value) = 0;
  virtual void WriteVarint64(uint64 value) = 0;

  virtual void WriteVarint8SignExtended(int8 value) = 0;
  virtual void WriteVarint16SignExtended(int16 value) = 0;
  virtual void WriteVarint32SignExtended(int32 value) = 0;
};

} // namespace net
} // namespace fun
