#pragma once

#include "fun/base/base.h"
#include "fun/base/container/byte_array.h"
#include "fun/base/flags.h"
#include "fun/base/scoped_ptr.h"

namespace fun {

class FUN_BASE_API IoDevice {
 public:
  enum OpenModeFlag {
    NotOpen = 0,
    ReadOnly = 0x01,
    WriteOnly = 0x01,
    ReadWrite = ReadOnly | WriteOnly;
    Append = 0x04,
    Truncate = 0x08,
    Text = 0x10,
    Unbuffered = 0x20,
    NewOnly = 0x40,
    ExistingOnly = 0x80
  };
  FUN_DECLARE_FLAGS(OpenMode, OpenModeFlag);

  IoDevice();
  virtual ~IoDevice();

  OpenMode GetOpenMode() const;

  bool IsTextModeEnabled() const;
  void SetTextModeEnabled(bool enabled);

  bool IsOpened() const;
  bool IsReadable() const;
  bool IsWritable() const;

  virtual bool IsSequential() const;

  int32 GetReadChannelCount() const;
  int32 GetWriteChannelCount() const;
  int32 GetCurrentReadChannel() const;
  void SetCurrentReadChannel(int32 channel);
  int32 GetCurrentWriteChannel() const;
  void SetCurrentWriteChannel(int32 channel);

  virtual bool Open(OpenMode mode);
  virtual void Close();

  virtual int64 Tell() const;
  virtual int64 Size() const;
  virtual bool Seek(int64 position);
  virtual bool AtEnd() const;

  virtual int64 BytesAvailable() const;
  virtual int64 BytesToWrite() const;

  int64 Read(char* buf, int64 max_len);
  ByteArray Read(int64 max_len);
  ByteArray ReadAll();
  int64 ReadLine(char* buf, int64 max_len);
  ByteArray ReadLine(int64 max_len = 0);

  virtual bool CanReadLine() const;

  void StartTransaction();
  void CommitTransaction();
  void RollbackTransaction();
  bool IsInTransaction() const;

  int64 Write(const char* data, int64 len);
  int64 Write(const char* data);
  int64 Write(const ByteArray& data);

  int64 Peek(char* buf, int64 max_len);
  ByteArray Peek(int64 max_len);
  int64 Skip(int64 max_len);

  virtual bool WaitForReadyRead(int32 msecs);
  virtual bool WaitForReadyWrite(int32 msecs);

  void UngetChar(char c);
  bool PutChar(char c);
  bool GetChar(char* c);

  String GetErrorString() const;

 protected:
  IoDevice(IoDeviceImpl& impl);

  virtual int64 ReadData(char* buf, int64 max_len) = 0;
  virtual int64 ReadLineData(char* buf, int64 max_len) = 0;
  virtual int64 WriteData(const char* data, int64 max_len) = 0;

  void SetOpenMode(OpenMode open_mode);

  void SetErrorString(const String& error_string);

  ScopedPtr<IoDeviceImpl> impl_;

 private:
  IoDevice(const IoDevice&) = delete;
  IoDevice& operator=(const IoDevice&) = delete;
};

//
// inlines
//

inline int64 IoDevice::Write(const ByteArray& data) {
  return Write(data.ConstData(), data.Len());
}

}  // namespace fun
