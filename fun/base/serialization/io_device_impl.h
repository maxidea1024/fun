#pragma once

#include "fun/base/base.h"

namespace fun {

#define FUN_IODEVICE_BUFFERSIZE  (16 * 1024)

class FUN_BASE_API IoDeviceImpl {
 public:
  IoDeviceImpl();
  virtual ~IoDeviceImpl();

  IoDevice::OpenMode open_mode_;
  String error_string_;

  Array<RingBuffer> read_buffers_;
  Array<RingBuffer> write_buffers_;

  class RingBufferRef {
   public:
    // wrap functions from RingBuffer
    inline void SetChunkSize(int32 size) { fun_check_ptr(buffer_); buffer_->SetChunkSize(size); }
    inline int32 GetChunkSize() const { fun_check_ptr(buffer_); return buffer_->GetChunkSize(); }
    inline int64 GetNextDataBlockSize() const { return (buffer_ ? buffer_->GetNextDataBlockSize() : int64(0)); }
    inline const char* GetReadPointer() const { return (buffer_ ? buffer_->GetReadPointer() : nullptr); }
    inline const char* GetReadPointerAtPosition(int64 position, int64& length) const { fun_check_ptr(buffer_); return buffer_->GetReadPointerAtPosition(position, length); }
    inline void Free(int64 bytes) { fun_check_ptr(buffer_); buffer_->Free(bytes); }
    inline char* Reverse(int64 bytes) { fun_check_ptr(buffer_); return buffer_->Reverse(bytes); }
    inline char* ReverseFront(int64 bytes) { fun_check_ptr(buffer_); return buffer_->ReverseFront(bytes); }
    inline void Truncate(int64 position) { fun_check_ptr(buffer_); buffer_->Truncate(position); }
    inline void Chop(int64 bytes) { fun_check_ptr(buffer_); buffer_->Chop(bytes); }
    inline bool IsEmpty() const { return !buffer_ || buffer_->IsEmpty(); }
    inline int32 GetChar() { return (buffer_ ? buffer_->GetChar() : -1); }
    inline void PutChar(char c) { fun_check_ptr(buffer_); buffer_->PutChar(c); }
    inline void UngetData(char c) { fun_check_ptr(buffer_); buffer_->UngetData(c); }
    inline int64 Size() const { return (buffer_ ? buffer_->Size() : int64(0)); }
    inline void Clear() { if (buffer_) buffer_->Clear(); }
    inline int64 IndexOf(char c) const { return (buffer_ ? buffer_->IndexOf(c, buffer_->Size()) : int64(-1)); }
    inline int64 IndexOf(char c, int64 max_len, int64 position = 0) const { return (buffer_ ? buffer_->IndexOf(c, max_len, position) : int64(-1)); }
    inline int64 Read(char* buf, int64 max_len) { return (buffer_ ? buffer_->Read(buf, max_len) : int64(0)); }
    inline ByteArray Read() { return (buffer_ ? buffer_->Read() : ByteArray()); }
    inline int64 Peek(char* buf, int64 max_len, int64 position = 0) const { return (buffer_ ? buffer_->Peek(buf, max_len, position) : int64(0)); }
    inline void Append(const char* data, int64 size) { fun_check_ptr(buffer_); buffer_->Append(data, size); }
    inline void Append(const ByteArray& array) { fun_check_ptr(buffer_); buffer_->Append(array); }
    inline int64 Skip(int64 length) { return (buffer_ ? buffer_->Skip(length) : int64(0)); }
    inline int64 ReadLine(char* buf, int64 max_len) { return (buffer_ ? buffer_->ReadLine(buf, max_len) : int64(-1)); }
    inline bool CanReadLine() const { return buffer_ && buffer_->CanReadLine(); }

   private:
    RingBuffer* buffer_;

    inline RingBufferRef() : buffer_(nullptr) {}

    friend class IoDeviceImpl;
  }

  RingBufferRef buffer_;
  RingBufferRef write_buffer_;
  int64 position_;
  int64 device_position_;

  int32 read_channel_count_;
  int32 write_channel_count_;
  int32 current_read_channel_;
  int32 current_write_channel_;
  int32 read_buffer_chunk_size_;
  int32 write_buffer_chunk_size_;
  int64 transaction_position_;
  bool is_in_transaction_;
  bool base_read_line_data_called_;

  virtual bool PutCharHelper(char c);

  enum AccessMode {
    Unset,
    Sequential,
    RandomAccess
  };
  mutable AccessMode access_mode_;

  inline bool IsSequential() const {
    if (access_mode_ == Unset) {
      access_mode_ = outer_->IsSequential() ? Sequential : RandomAccess;
    }
    return access_mode_ == Sequential;
  }

  inline bool IsBufferEmpty() const {
    return buffer_.IsEmpty() || (is_in_transaction_ && IsSequential()
                  && transaction_position_ == buffer_.Size());
  }

  bool AllWriteBuffersEmpty() const;

  void SeekBuffer(int64 new_pos);

  inline void SetCurrentReadChannel(int32 channel) {
    buffer_.buffer_ = (channel < read_buffers_.Count() ? &read_buffers_[channel] : nullptr);
    current_read_channel_ = channel;
  }

  inline void SetCurrentWriteChannel(int32 channel) {
    write_buffer_.buffer_ = (channel < write_buffers_.Count() ? &write_buffers_[channel] : nullptr);
    current_write_channel_ = channel;
  }

  void SetReadChannelCount(int32 count);

  void SetWriteChannelCount(int32 count);

  int64 Read(char* buf, int64 max_len, bool peeking = false);

  virtual int64 Peek(char* buf, int64 max_len);

  virtual ByteArray Peek(int64 max_len);

  int64 SkipByReading(int64 max_len);

  // ### Qt6: consider replacing with a protected virtual IoDevice::SkipData().
  virtual int64 Skip(int64 max_len);

  IoDevice* outer_;
};

} // namespace fun
