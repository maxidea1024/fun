#include "fun/base/serialization/io_device.h"

namespace fun {

IoDeviceImpl::IoDeviceImpl()
  : open_mode_(IoDevice::NotOpen),
    position_(0),
    device_position_(0),
    read_channel_count_(0),
    write_channel_count_(0),
    current_read_channel_(0),
    current_write_channel_(0),
    read_buffer_chunk_size_(FUN_IODEVICE_BUFFERSIZE),
    write_buffer_chunk_size_(0),
    transaction_position_(0),
    is_in_transaction_(false),
    base_read_line_data_called_(false),
    access_mode_(Unset),
    outer_(nullptr) {}

IoDeviceImpl::~IoDeviceImpl() {}

IoDevice::IoDevice() : impl_(new IoDeviceImpl) {
  impl_->outer_ = this;
}

IoDevice::IoDevice(IoDeviceImpl& impl) : impl_(&impl) {
  impl_->outer_ = this;
}

IoDevice::~IoDevice() {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::~IoDevice()\n", this);
#endif
}

bool IoDevice::IsSequential() const {
  return false;
}

IoDevice::OpenMode IoDevice::GetOpenMode() const {
  return impl_->open_mode_;
}

void IoDevice::SetOpenMode(OpenMode open_mode) {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::SetOpenMode(0x%x)\n", this, int32(open_mode));
#endif

  impl_->open_mode_ = open_mode;
  impl_->access_mode_ = IoDeviceImpl::Unset;
  impl_->SetReadChannelCount(IsReadable() ? MathBase::Max(impl_->read_channel_count_, 1) : 0);
  impl_->SetWriteChannelCount(IsWritable() ? MathBase::Max(impl_->write_channel_count_, 1) : 0);
}

void IoDevice::SetTextModeEnabled(bool enabled) {
  if (!IsOpened()) {
    CheckWarnMessage(this, "SetTextModeEnabled", "The device is not open");
    return;
  }

  if (enabled) {
    impl_->open_mode_ |= Text;
  } else {
    impl_->open_mode_ &= ~Text;
  }
}

bool IoDevice::IsTextModeEnabled() const {
  return impl_->open_mode_ & Text;
}

bool IoDevice::IsOpened() const {
  return impl_->open_mode_ != NotOpen;
}

bool IoDevice::IsReadable() const {
  return !!(GetOpenMode() & ReadOnly);
}

bool IoDevice::IsWritable() const {
  return !!(GetOpenMode() & WriteOnly);
}

int32 IoDevice::GetReadChannelCount() const {
  return impl_->read_channel_count_;
}

int32 IoDevice::GetWriteChannelCount() const {
  return impl_->write_channel_count_;
}

int32 IoDevice::GetCurrentReadChannel() const {
  return impl_->current_read_channel_;
}

void IoDevice::SetCurrentReadChannel(int32 channel) {
  if (impl_->is_in_transaction_) {
    CheckWarnMessage(this, "SetCurrentReadChannel", "Failed due to read transaction being in progress");
    return;
  }

#if defined FUN_IODEVICE_DEBUG
  qDebug("%p IoDevice::SetCurrentReadChannel(%d), impl_->current_read_channel_ = %d, impl_->read_channel_count_ = %d\n",
       this, channel, impl_->current_read_channel_, impl_->read_channel_count_);
#endif

  impl_->SetCurrentReadChannel(channel);
}

void IoDeviceImpl::SetReadChannelCount(int32 count) {
  if (count > read_buffers_.Count()) {
    read_buffers_.insert(read_buffers_.end(), count - read_buffers_.Count(),
                        RingBuffer(read_buffer_chunk_size_));
  } else {
    read_buffers_.Resize(count);
  }
  read_channel_count_ = count;
  SetCurrentReadChannel(current_read_channel_);
}

int32 IoDevice::GetCurrentWriteChannel() const {
  return impl_->current_write_channel_;
}

void IoDevice::SetCurrentWriteChannel(int32 channel) {
#if defined FUN_IODEVICE_DEBUG
  qDebug("%p IoDevice::SetCurrentWriteChannel(%d), impl_->current_write_channel_ = %d, impl_->write_channel_count_ = %d\n",
        this, channel, impl_->current_write_channel_, impl_->write_channel_count_);
#endif

  impl_->SetCurrentWriteChannel(channel);
}

void IoDeviceImpl::SetWriteChannelCount(int32 count) {
  if (count > write_buffers_.Count()) {
    // If write_buffer_chunk_size_ is zero (default value), we don't use
    // IoDevice's Write buffers.
    if (write_buffer_chunk_size_ != 0) {
      write_buffers_.insert(write_buffers_.end(), count - write_buffers_.Count(),
                            RingBuffer(write_buffer_chunk_size_));
    }
  } else {
    write_buffers_.Resize(count);
  }
  write_channel_count_ = count;
  SetCurrentWriteChannel(current_write_channel_);
}

bool IoDeviceImpl::AllWriteBuffersEmpty() const {
  for (const RingBuffer& buf : write_buffers_) {
    if (!buf.IsEmpty()) {
      return false;
    }
  }

  return true;
}

bool IoDevice::Open(OpenMode open_mode) {
  impl_->open_mode_ = open_mode;
  impl_->position_ = (open_mode & Append) ? Size() : int64(0);
  impl_->access_mode_ = IoDeviceImpl::Unset;
  impl_->read_buffers_.Clear();
  impl_->write_buffers_.Clear();
  impl_->SetReadChannelCount(IsReadable() ? 1 : 0);
  impl_->SetWriteChannelCount(IsWritable() ? 1 : 0);
  impl_->error_string_.Clear();

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Open(0x%x)\n", this, uint32(open_mode));
#endif

  return true;
}

void IoDevice::Close() {
  if (impl_->open_mode_ == NotOpen) {
    return;
  }

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Close()\n", this);
#endif

  impl_->open_mode_ = NotOpen;
  impl_->position_ = 0;
  impl_->is_in_transaction_ = false;
  impl_->transaction_position_ = 0;
  impl_->SetReadChannelCount(0);
  // Do not clear Write buffers to allow delayed close in sockets
  impl_->write_channel_count_ = 0;
}

int64 IoDevice::Tell() const {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Tell() == %lld\n", this, impl_->position_);
#endif

  return impl_->position_;
}

int64 IoDevice::Size() const {
  return impl_->IsSequential() ? BytesAvailable() : int64(0);
}

bool IoDevice::Seek(int64 position) {
  if (impl_->IsSequential()) {
    CheckWarnMessage(this, "Seek", "Cannot call Seek on a sequential device");
    return false;
  }

  if (impl_->open_mode_ == NotOpen) {
    CheckWarnMessage(this, "Seek", "The device is not open");
    return false;
  }

  if (position < 0) {
    fun_log(Warning, "IoDevice::Seek: Invalid position_: %lld", position);
    return false;
  }

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Seek(%lld), before: impl_->position_ = %lld, impl_->buffer_.Len() = %lld\n",
        this, position_, impl_->position_, impl_->buffer_.Count());
#endif

  impl_->device_position_ = position;
  impl_->SeekBuffer(position_);

#if defined FUN_IODEVICE_DEBUG
  printf("%p \tafter: impl_->position_ == %lld, impl_->buffer_.Count() == %lld\n",
        this, impl_->position_, impl_->buffer_.Count());
#endif

  return true;
}

void IoDeviceImpl::SeekBuffer(int64 new_pos) {
  const int64 offset = new_pos - position_;
  position_ = new_pos;

  if (offset < 0 || offset >= buffer_.Len()) {
    // When seeking backwards, an operation that is only allowed for
    // random-access devices, the buffer is cleared. The next read
    // operation will then refill the buffer.
    buffer_.Clear();
  } else {
    buffer_.Free(offset);
  }
}

bool IoDevice::AtEnd() const {
  const bool at_end = (impl_->open_mode_ == NotOpen || (impl_->IsBufferEmpty() && BytesAvailable() == 0));

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::AtEnd() returns %s, impl_->open_mode_ == %d, impl_->position_ == %lld\n", this,
          at_end ? "true" : "false", int32(impl_->open_mode_), impl_->position_);
#endif

  return at_end;
}

bool IoDevice::Reset() {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Reset()\n", this);
#endif

  return Seek(0);
}

int64 IoDevice::BytesAvailable() const {
  if (!impl_->IsSequential()) {
    return MathBase::Max(Size() - impl_->position_, int64(0));
  } else {
    return impl_->buffer_.Len() - impl_->transaction_position_;
  }
}

int64 IoDevice::BytesToWrite() const {
  return impl_->write_buffer_.Len();
}

int64 IoDevice::Read(char* buf, int64 max_len) {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Read(%p, %lld), impl_->position_ = %lld, impl_->buffer_.Count() = %lld\n",
          this, buf, max_len, impl_->position_, impl_->buffer_.Len());
#endif

  const bool sequential = impl_->IsSequential();

  // Short-cut for GetChar(), unless we need to keep the data in the buffer.
  if (max_len == 1 && !(sequential && impl_->is_in_transaction_)) {
    int32 chint;
    while ((chint = impl_->buffer_.GetChar()) != -1) {
      if (!sequential) {
        ++impl_->position_;
      }

      char c = char(uchar(chint));
      if (c == '\r' && (impl_->open_mode_ & Text)) {
        continue;
      }

      *buf = c;

#if defined FUN_IODEVICE_DEBUG
      printf("%p \tread 0x%hhx (%c) returning 1 (shortcut)\n",
            this, int32(c), isprint(c) ? c : '?');
#endif

      if (impl_->buffer_.IsEmpty()) {
        ReadData(buf, 0);
      }

      return 1;
    }
  }

  CHECK_MAXLEN(Read, int64(-1));
  CHECK_READABLE(Read, int64(-1));

  const int64 readed_len = impl_->Read(buf, max_len);

#if defined FUN_IODEVICE_DEBUG
  printf("%p \treturning %lld, impl_->position_ == %lld, impl_->buffer_.Count() == %lld\n",
          this, readed_len, impl_->position_, impl_->buffer_.Count());
  if (readed_len > 0) {
    debugBinaryString(buf - readed_len, readed_len);
  }
#endif

  return readed_len;
}

int64 IoDeviceImpl::Read(char* buf, int64 max_len, bool peeking) {
  const bool buffered = (open_mode_ & IoDevice::Unbuffered) == 0;
  const bool sequential = IsSequential();
  const bool keep_data_in_buffer = sequential
                  ? peeking || is_in_transaction_
                  : peeking && buffered;
  const int64 saved_pos = position_;
  int64 total_readed_len = 0;
  bool made_buffer_reads_only = true;
  bool device_at_eof = false;
  char* read_ptr = buf;
  int64 buffer_pos = (sequential && is_in_transaction_) ? transaction_position_ : int64(0);
  for (;;) {
    // Try reading from the buffer.
    int64 buffer_read_chunk_size = keep_data_in_buffer
                   ? buffer.Peek(buf, max_len, buffer_pos)
                   : buffer.Read(buf, max_len);
    if (buffer_read_chunk_size > 0) {
      buffer_pos += buffer_read_chunk_size;

      if (!sequential) {
        position_ += buffer_read_chunk_size;
      }

#if defined FUN_IODEVICE_DEBUG
      printf("%p \treading %lld bytes from buffer into position %lld\n",
            outer_, buffer_read_chunk_size, total_readed_len);
#endif

      total_readed_len += buffer_read_chunk_size;
      buf += buffer_read_chunk_size;
      max_len -= buffer_read_chunk_size;
    }

    if (max_len > 0 && !device_at_eof) {
      int64 readed_len_from_device = 0;
      // Make sure the device is positioned correctly.
      if (sequential || position_ == device_position_ || outer_->Seek(position_)) {
        made_buffer_reads_only = false; // fix ReadData attempt
        if ((!buffered || max_len >= read_buffer_chunk_size_) && !keep_data_in_buffer) {
          // Read big chunk directly to output buffer
          readed_len_from_device = outer_->ReadData(buf, max_len);
          device_at_eof = (readed_len_from_device != max_len);
#if defined FUN_IODEVICE_DEBUG
          printf("%p \treading %lld bytes from device (total %lld)\n",
                  outer_, readed_len_from_device, total_readed_len);
#endif
          if (readed_len_from_device > 0) {
            total_readed_len += readed_len_from_device;
            buf += readed_len_from_device;
            max_len -= readed_len_from_device;
            if (!sequential)
            {
              position_ += readed_len_from_device;
              device_position_ += readed_len_from_device;
            }
          }
        } else {
          // Do not read more than max_len on unbuffered devices
          const int64 bytes_to_buffer = (buffered || read_buffer_chunk_size_ < max_len)
                                        ? int64(read_buffer_chunk_size_)
                                        : max_len;
          // Try to fill IoDevice buffer by single read
          readed_len_from_device = outer_->ReadData(buffer.Reverse(bytes_to_buffer), bytes_to_buffer);
          device_at_eof = (readed_len_from_device != bytes_to_buffer);
          buffer.Chop(bytes_to_buffer - MathBase::Max(int64(0), readed_len_from_device));
          if (readed_len_from_device > 0) {
            if (!sequential)
            {
              device_position_ += readed_len_from_device;
            }
#if defined FUN_IODEVICE_DEBUG
            printf("%p \treading %lld from device into buffer\n", outer_, readed_len_from_device);
#endif
            continue;
          }
        }
      } else {
        readed_len_from_device = -1;
      }

      if (readed_len_from_device < 0 && total_readed_len == 0) {
        // error and we haven't read anything: return immediately
        return int64(-1);
      }
    }

    if ((open_mode_ & IoDevice::Text) && read_ptr < buf) {
      const char* end_ptr = buf;

      // optimization to avoid initial self-assignment
      while (*read_ptr != '\r') {
        if (++read_ptr == end_ptr) {
          break;
        }
      }

      char* write_ptr = read_ptr;

      while (read_ptr < end_ptr) {
        char c = *read_ptr++;
        if (c != '\r') {
          *write_ptr++ = c;
        } else {
          --total_readed_len;
          --buf;
          ++max_len;
        }
      }

      // Make sure we get more data if there is room for more. This
      // is very important for when someone seeks to the start of a
      // '\r\n' and reads one character - they should get the '\n'.
      read_ptr = buf;
      continue;
    }

    break;
  }

  // Restore positions after reading
  if (keep_data_in_buffer) {
    if (peeking) {
      position_ = saved_pos; // does nothing on sequential devices
    } else {
      transaction_position_ = buffer_pos;
    }
  } else if (peeking) {
    SeekBuffer(saved_pos); // unbuffered random-access device
  }

  if (made_buffer_reads_only && IsBufferEmpty()) {
    outer_->ReadData(buf, 0);
  }

  return total_readed_len;
}

ByteArray IoDevice::Read(int64 max_len) {
  ByteArray result;

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Read(%lld), impl_->position_ = %lld, impl_->buffer_.Len() = %lld\n",
        this, max_len, impl_->position_, impl_->buffer_.Len());
#endif

  // Try to prevent the data from being copied, if we have a chunk
  // with the same size in the read buffer.
  if (max_len == impl_->buffer_.GetNextDataBlockSize() && !impl_->is_in_transaction_
      && (impl_->open_mode_ & (IoDevice::ReadOnly | IoDevice::Text)) == IoDevice::ReadOnly) {
    result = impl_->buffer_.Read();

    if (!impl_->IsSequential()) {
      impl_->position_ += max_len;
    }

    if (impl_->buffer_.IsEmpty()) {
      ReadData(nullptr, 0);
    }

    return result;
  }

  CHECK_MAXLEN(Read, result);
  CHECK_MAXBYTEARRAYSIZE(Read);

  result.Resize(int32(max_len));
  const int64 readed_len = Read(result.MutableData(), result.Len());

  if (readed_len <= 0) {
    result.Clear();
  } else {
    result.Resize(int32(readed_len));
  }

  return result;
}

ByteArray IoDevice::ReadAll() {
#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::ReadAll(), impl_->position_ = %lld, impl_->buffer_.Count() = %lld\n",
        this, impl_->position_, impl_->buffer_.Count());
#endif

  ByteArray result;
  int64 total_readed_len = (impl_->IsSequential() ? int64(0) : Size());
  if (total_readed_len == 0) {
    // Size is unknown, read incrementally.
    int64 read_chunk_size = MathBase::Max(int64(impl_->read_buffer_chunk_size_),
                  impl_->IsSequential() ? (impl_->buffer_.Count() - impl_->transaction_position_)
                            : impl_->buffer_.Count());
    int64 readed_len;
    do {
      if (total_readed_len + read_chunk_size >= MaxByteArraySize) {
        // If Resize would fail, don't read more, return what we have.
        break;
      }
      result.Resize(total_readed_len + read_chunk_size);
      readed_len = Read(result.MutableData() + total_readed_len, read_chunk_size);
      if (readed_len > 0 || total_readed_len == 0) {
        total_readed_len += readed_len;
        read_chunk_size = impl_->read_buffer_chunk_size_;
      }
    } while (readed_len > 0);
  } else {
    // Read it all in one go.
    // If Resize fails, don't read anything.
    total_readed_len -= impl_->position_;
    if (total_readed_len >= MaxByteArraySize) {
      return ByteArray();
    }
    result.Resize(total_readed_len);
    total_readed_len = Read(result.MutableData(), total_readed_len);
  }

  if (total_readed_len <= 0) {
    result.Clear();
  } else {
    result.Resize(int32(total_readed_len));
  }

  return result;
}

int64 IoDevice::ReadLine(char* buf, int64 max_len) {
  if (max_len < 2) {
    CheckWarnMessage(this, "ReadLine", "Called with max_len < 2");
    return int64(-1);
  }

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::ReadLine(%p, %lld), impl_->position_ = %lld, impl_->buffer_.Len() = %lld\n",
        this, buf, max_len, impl_->position_, impl_->buffer_.Len());
#endif

  // Leave room for a '\0'
  --max_len;

  const bool sequential = impl_->IsSequential();
  const bool keep_data_in_buffer = sequential && impl_->is_in_transaction_;

  int64 total_readed_len = 0;
  if (keep_data_in_buffer) {
    if (impl_->transaction_position_ < impl_->buffer_.Len()) {
      // Peek line from the specified position
      const int64 i = impl_->buffer_.IndexOf('\n', max_len, impl_->transaction_position_);
      total_readed_len = impl_->buffer_.Peek(buf, i >= 0 ? (i - impl_->transaction_position_ + 1) : max_len,
                                            impl_->transaction_position_);
      impl_->transaction_position_ += total_readed_len;
      if (impl_->transaction_position_ == impl_->buffer_.Len()) {
        ReadData(buf, 0);
      }
    }
  } else if (!impl_->buffer_.IsEmpty()) {
    // RingBuffer::ReadLine() terminates the line with '\0'
    total_readed_len = impl_->buffer_.ReadLine(buf, max_len + 1);

    if (impl_->buffer_.IsEmpty()) {
      ReadData(buf, 0);
    }

    if (!sequential) {
      impl_->position_ += total_readed_len;
    }
  }

  if (total_readed_len) {
#if defined FUN_IODEVICE_DEBUG
    printf("%p \tread from buffer: %lld bytes, last character read: %hhx\n", this,
          total_readed_len, buf[total_readed_len - 1]);
    debugBinaryString(data, int32(total_readed_len));
#endif
    if (buf[total_readed_len - 1] == '\n') {
      if (impl_->open_mode_ & Text) {
        // RingBuffer::ReadLine() isn't Text aware.
        if (total_readed_len > 1 && buf[total_readed_len - 2] == '\r') {
          --total_readed_len;
          buf[total_readed_len - 1] = '\n';
        }
      }
      buf[total_readed_len] = '\0';
      return total_readed_len;
    }
  }

  if (impl_->position_ != impl_->device_position_ && !sequential && !Seek(impl_->position_)) {
    return int64(-1);
  }

  impl_->base_read_line_data_called_ = false;
  // Force base implementation for transaction on sequential device
  // as it stores the data in internal buffer automatically.
  int64 readed_len = keep_data_in_buffer
                     ? IoDevice::ReadLineData(buf + total_readed_len, max_len - total_readed_len)
                     : ReadLineData(buf + total_readed_len, max_len - total_readed_len);

#if defined FUN_IODEVICE_DEBUG
  printf("%p \tread from ReadLineData: %lld bytes, total_readed_len = %lld bytes\n", this,
        readed_len, total_readed_len);
  if (readed_len > 0) {
    debugBinaryString(buf, int32(total_readed_len + readed_len));
  }
#endif

  if (readed_len < 0) {
    buf[total_readed_len] = '\0';
    return total_readed_len ? total_readed_len : -1;
  }
  total_readed_len += readed_len;
  if (!impl_->base_read_line_data_called_ && !sequential) {
    impl_->position_ += readed_len;
    // If the base implementation was not called, then we must
    // assume the device position is invalid and force a Seek.
    impl_->device_position_ = int64(-1);
  }
  buf[total_readed_len] = '\0';

  if (impl_->open_mode_ & Text) {
    if (total_readed_len > 1 && buf[total_readed_len - 1] == '\n' && buf[total_readed_len - 2] == '\r') {
      buf[total_readed_len - 2] = '\n';
      buf[total_readed_len - 1] = '\0';
      --total_readed_len;
    }
  }

#if defined FUN_IODEVICE_DEBUG
  printf("%p \treturning %lld, impl_->position_ = %lld, impl_->buffer_.Len() = %lld, Size() = %lld\n",
       this, total_readed_len, impl_->position_, impl_->buffer_.Len(), Size());
  debugBinaryString(buf, int32(total_readed_len));
#endif

  return total_readed_len;
}

ByteArray IoDevice::ReadLine(int64 max_len) {
  ByteArray result;

  CHECK_MAXLEN(ReadLine, result);
  CHECK_MAXBYTEARRAYSIZE(ReadLine);

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::ReadLine(%lld), impl_->position_ = %lld, impl_->buffer_.Len() = %lld\n",
        this, max_len, impl_->position_, impl_->buffer_.Len());
#endif

  result.Resize(int32(max_len));
  int64 total_readed_len = 0;
  if (!result.Len()) {
    // If Resize fails or max_len == 0, read incrementally
    if (max_len == 0) {
      max_len = MaxByteArraySize - 1;
    }

    // The first iteration needs to leave an extra byte for the terminating null
    result.Resize(1);

    int64 readed_len;
    do {
      result.Resize(int32(MathBase::Min(max_len, int64(result.Len() + impl_->read_buffer_chunk_size_))));
      readed_len = ReadLine(result.MutableData() + total_readed_len, result.Len() - total_readed_len);
      if (readed_len > 0 || total_readed_len == 0) {
        total_readed_len += readed_len;
      }
    } while (readed_len == impl_->read_buffer_chunk_size_
              && result[int32(total_readed_len - 1)] != '\n');
  } else {
    total_readed_len = ReadLine(result.MutableData(), result.Len());
  }

  if (total_readed_len <= 0) {
    result.Clear();
  } else {
    result.Resize(total_readed_len);
  }

  return result;
}

// Slow...
int64 IoDevice::ReadLineData(char* buf, int64 max_len) {
  int64 total_readed_len = 0;
  char c;
  int32 last_read_return = 0;
  impl_->base_read_line_data_called_ = true;

  while (total_readed_len < max_len && (last_read_return = Read(&c, 1)) == 1) {
    *buf++ = c;
    ++total_readed_len;
    if (c == '\n') {
      break;
    }
  }

#if defined FUN_IODEVICE_DEBUG
  printf( "%p IoDevice::ReadLineData(%p, %lld), impl_->position_ = %lld, impl_->buffer_.Len() = %lld, "
          "returns %lld\n", this, buf, max_len, impl_->position_, impl_->buffer_.Len(), total_readed_len);
#endif

  if (last_read_return != 1 && total_readed_len == 0) {
    return IsSequential() ? last_read_return : -1;
  }

  return total_readed_len;
}

bool IoDevice::CanReadLine() const {
  return impl_->buffer_.IndexOf('\n', impl_->buffer_.Len(),
               impl_->IsSequential() ? impl_->transaction_position_ : int64(0)) >= 0;
}

void IoDevice::StartTransaction() {
  if (impl_->is_in_transaction_) {
    CheckWarnMessage(this, "StartTransaction", "Called while transaction already in progress");
    return;
  }

  impl_->transaction_position_ = impl_->position_;
  impl_->is_in_transaction_ = true;
}

void IoDevice::CommitTransaction() {
  if (!impl_->is_in_transaction_) {
    CheckWarnMessage(this, "CommitTransaction", "Called while no transaction in progress");
    return;
  }

  if (impl_->IsSequential()) {
    impl_->buffer_.Free(impl_->transaction_position_);
  }

  impl_->is_in_transaction_ = false;
  impl_->transaction_position_ = 0;
}

void IoDevice::RollbackTransaction() {
  if (!impl_->is_in_transaction_) {
    CheckWarnMessage(this, "RollbackTransaction", "Called while no transaction in progress");
    return;
  }

  if (!impl_->IsSequential()) {
    impl_->SeekBuffer(impl_->transaction_position_);
  }

  impl_->is_in_transaction_ = false;
  impl_->transaction_position_ = 0;
}

bool IoDevice::IsInTransaction() const {
  return impl_->is_in_transaction_;
}

int64 IoDevice::Write(const char* data, int64 max_len) {
  CHECK_WRITABLE(Write, int64(-1));
  CHECK_MAXLEN(Write, int64(-1));

  const bool sequential = impl_->IsSequential();
  // Make sure the device is positioned correctly.
  if (impl_->position_ != impl_->device_position_ && !sequential && !Seek(impl_->position_)) {
    return int64(-1);
  }

#if defined(FUN_PLATFORM_WINDOWS_FAMILY)
  if (impl_->open_mode_ & Text) {
    const char* end_of_data = data + max_len;
    const char* start_of_block = data;

    int64 total_written_len = 0;
    const int64 saved_pos = impl_->position_;

    for (;;) {
      const char* end_of_block = start_of_block;
      while (end_of_block < end_of_data && *end_of_block != '\n') {
        ++end_of_block;
      }

      int64 block_size = end_of_block - start_of_block;
      if (block_size > 0) {
        int64 ret = WriteData(start_of_block, block_size);
        if (ret <= 0) {
          if (total_written_len && !sequential) {
            impl_->buffer_.Skip(impl_->position_ - saved_pos);
          }

          return total_written_len ? total_written_len : ret;
        }

        if (!sequential) {
          impl_->position_ += ret;
          impl_->device_position_ += ret;
        }

        total_written_len += ret;
      }

      if (end_of_block == end_of_data) {
        break;
      }

      int64 ret = WriteData("\r\n", 2);
      if (ret <= 0) {
        if (total_written_len && !sequential) {
          impl_->buffer_.Skip(impl_->position_ - saved_pos);
        }

        return total_written_len ? total_written_len : ret;
      }
      if (!sequential) {
        impl_->position_ += ret;
        impl_->device_position_ += ret;
      }
      ++total_written_len;

      start_of_block = end_of_block + 1;
    }

    if (total_written_len && !sequential) {
      impl_->buffer_.Skip(impl_->position_ - saved_pos);
    }

    return total_written_len;
  }
#endif // defined(FUN_PLATFORM_WINDOWS_FAMILY)

  int64 written_len = WriteData(data, max_len);
  if (!sequential && written_len > 0) {
    impl_->position_ += written_len;
    impl_->device_position_ += written_len;
    impl_->buffer_.Skip(written_len);
  }

  return written_len;
}

int64 IoDevice::Write(const char* data) {
  return Write(data, qstrlen(data));
}

void IoDevice::UngetData(char c) {
  CHECK_READABLE(Read, FUN_VOID);

  if (impl_->is_in_transaction_) {
    CheckWarnMessage(this, "UngetData", "Called while transaction is in progress");
    return;
  }

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::UngetData(0x%hhx '%c')\n", this, c, isprint(c) ? c : '?');
#endif

  impl_->buffer_.UngetData(c);

  // Random-access device일 경우에는 뒤로 1바이트 이동.
  if (!impl_->IsSequential()) {
    --impl_->position_;
  }
}

bool IoDevice::PutChar(char c) {
  return impl_->PutCharHelper(c);
}

bool IoDeviceImpl::PutCharHelper(char c) {
  return outer_->Write(&c, 1) == 1;
}

int64 IoDeviceImpl::Peek(char* buf, int64 max_len) {
  return Read(buf, max_len, true);
}

ByteArray IoDeviceImpl::Peek(int64 max_len) {
  ByteArray result(max_len, Uninitialized);

  const int64 readed_len = Read(result.MutableData(), max_len, true);
  if (readed_len < max_len) {
    if (readed_len <= 0) {
      result.Clear();
    } else {
      result.Resize(readed_len);
    }
  }

  return result;
}

bool IoDevice::GetChar(char* c) {
  // readability checked in Read()
  char ignorant;
  return Read(c ? c : &ignorant, 1) == 1;
}

int64 IoDevice::Peek(char* buf, int64 max_len) {
  CHECK_MAXLEN(Peek, int64(-1));
  CHECK_READABLE(Peek, int64(-1));

  return impl_->Peek(buf, max_len);
}

ByteArray IoDevice::Peek(int64 max_len) {
  CHECK_MAXLEN(Peek, ByteArray());
  CHECK_MAXBYTEARRAYSIZE(Peek);
  CHECK_READABLE(Peek, ByteArray());

  return impl_->Peek(max_len);
}

int64 IoDevice::Skip(int64 max_len) {
  CHECK_MAXLEN(Skip, int64(-1));
  CHECK_READABLE(Skip, int64(-1));

  const bool sequential = impl_->IsSequential();

#if defined FUN_IODEVICE_DEBUG
  printf("%p IoDevice::Skip(%lld), impl_->position_ = %lld, impl_->buffer_.Len() = %lld\n",
        this, max_len, impl_->position_, impl_->buffer_.Len());
#endif

  if ((sequential && impl_->is_in_transaction_) || (impl_->open_mode_ & IoDevice::Text) != 0) {
    return impl_->SkipByReading(max_len);
  }

  // First, Skip over any data in the internal buffer.
  int64 total_skipped_len = 0;
  if (!impl_->buffer_.IsEmpty()) {
    total_skipped_len = impl_->buffer_.Skip(max_len);

#if defined FUN_IODEVICE_DEBUG
    printf("%p \tskipping %lld bytes in buffer\n", this, total_skipped_len);
#endif

    if (!sequential) {
      impl_->position_ += total_skipped_len;
    }

    if (impl_->buffer_.IsEmpty()) {
      ReadData(nullptr, 0);
    }

    if (total_skipped_len == max_len) {
      return total_skipped_len;
    }

    max_len -= total_skipped_len;
  }

  // Try to Seek on random-access device. At this point,
  // the internal read buffer is empty.
  if (!sequential) {
    const int64 skippable_len = MathBase::Min(Size() - impl_->position_, max_len);

    // If the size is unknown or file position is at the end,
    // fall back to reading below.
    if (skippable_len > 0) {
      if (!Seek(impl_->position_ + skippable_len)) {
        return total_skipped_len ? total_skipped_len : int64(-1);
      }

      if (skippable_len == max_len) {
        return total_skipped_len + skippable_len;
      }

      total_skipped_len += skippable_len;
      max_len -= skippable_len;
    }
  }

  const int64 skipped_len = impl_->Skip(max_len);
  if (total_skipped_len == 0) {
    return skipped_len;
  }

  if (skipped_len == -1) {
    return total_skipped_len;
  }

  return total_skipped_len + skipped_len;
}

int64 IoDeviceImpl::SkipByReading(int64 max_len) {
  int64 total_readed_len = 0;
  do {
    char dummy[4096];
    const int64 readable_len = MathBase::Min<int64>(max_len, sizeof dummy);
    const int64 readed_len = Read(dummy, readable_len);

    // Do not try again, if we got less data.
    if (readed_len != readable_len) {
      if (total_readed_len == 0) {
        return readed_len;
      }

      if (readed_len == -1) {
        return total_readed_len;
      }

      return total_readed_len + readed_len;
    }

    total_readed_len += readed_len;
    max_len -= readed_len;
  } while (max_len > 0);

  return total_readed_len;
}

int64 IoDeviceImpl::Skip(int64 max_len) {
  // Base implementation discards the data by reading into the dummy buffer.
  // It's slow, but this works for all types of devices. Subclasses can
  // reimplement this function to improve on that.
  return SkipByReading(max_len);
}

bool IoDevice::WaitForReadyRead(int32 msecs) {
  FUN_UNUSED(msecs);
  return false;
}

bool IoDevice::WaitForBytesWritten(int32 msecs) {
  FUN_UNUSED(msecs);
  return false;
}

void IoDevice::SetErrorString(const String& str) {
  impl_->error_string_ = str;
}

String IoDevice::GetErrorString() const {
  if (impl_->error_string_.IsEmpty()) {
#ifdef QT_NO_QOBJECT
    return Latin1String(QT_TRANSLATE_NOOP(IoDevice, "Unknown error"));
#else
    return tr("Unknown error");
#endif
  }
  return impl_->error_string_;
}


int32 qt_subtract_from_timeout(int32 timeout, int32 elapsed) {
  if (timeout == -1) {
    return -1;
  }

  timeout = timeout - elapsed;
  return timeout < 0 ? 0 : timeout;
}


#if !defined(FUN_NO_DEBUG_STREAM)
QDebug operator << (QDebug debug, IoDevice::OpenMode modes) {
  debug << "OpenMode(";
  StringList mode_list;
  if (modes == IoDevice::NotOpen) {
    mode_list << Latin1String("NotOpen");
  } else {
    if (modes & IoDevice::ReadOnly) {
      mode_list << Latin1String("ReadOnly");
    }

    if (modes & IoDevice::WriteOnly) {
      mode_list << Latin1String("WriteOnly");
    }

    if (modes & IoDevice::Append) {
      mode_list << Latin1String("Append");
    }

    if (modes & IoDevice::Truncate) {
      mode_list << Latin1String("Truncate");
    }

    if (modes & IoDevice::Text) {
      mode_list << Latin1String("Text");
    }

    if (modes & IoDevice::Unbuffered) {
      mode_list << Latin1String("Unbuffered");
    }
  }
  std::sort(mode_list.begin(), mode_list.end());
  debug << mode_list.Join(Latin1Char('|'));
  debug << ')';
  return debug;
}
#endif

} // namespace fun
