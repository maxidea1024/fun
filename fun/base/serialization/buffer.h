#pragma once

#include "fun/base/base.h"
#include "fun/base/container/byte_array.h"
#include "fun/base/scoped_ptr.h"
#include "fun/base/serialization/io_device.h"

namespace fun {

class FUN_BASE_API Buffer : public IoDevice {
 public:
  Buffer();
  explicit Buffer(ByteArray* buf);
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer& operator = (const Buffer&) = delete;

  ByteArray& GetBuffer();
  const ByteArray& GetBuffer() const;
  void SetBuffer(ByteArray* buf);

  void SetData(const ByteArray& data);
  void SetData(const char* data, int32 len);
  const ByteArray& GetData() const;

  // IoDevice interface.
  bool Open(OpenMode open_mode) override;
  void Close() override;
  int64 Size() const override;
  int64 Tell() const override;
  bool Seek(int64 position) override;
  bool AtEnd() const override;
  bool CanReadLine() const override;

 protected:
  int64 ReadData(char* buf, int64 max_len) override;
  int64 WriteData(const char* data, int64 len) override;

 private:
  ScopedPtr<BufferImpl> impl_;
};

} // namespace fun
