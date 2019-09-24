#include "fun/base/serialization/io_device_impl.h"

namespace fun {

class BufferImpl : public IoDeviceImpl {
 public:
  ByteArray* buffer_;
  ByteArray default_buffer_;

  BufferImpl() : buffer_(nullptr) {}

  ~BufferImpl() {}

  int64 Peek(char* buf, int64 max_len) override {
    int64 readable_len = MathBase::Min(max_len, static_cast<int64>(buffer_->Count()) - position_);
    UnsafeMemory::Memcpy(buf, buffer_->ConstData() + position_, readable_len);
    return readable_len;
  }

  ByteArray Peek(int64 max_len) override {
    int64 readable_len = MathBase::Min(max_len, static_cast<int64>(buffer_->Count()) - position_);
    if (position_ == 0 && max_len >= buffer_->Count()) {
      return *buf;
    }

    return ByteArray(buffer_->ConstData() + position_, readable_len);
  }
};


//
// Buffer
//

Buffer::Buffer() : IoDevice(*new BufferImpl) {
  impl_->buffer_ = &impl_->default_buffer_;
}

Buffer::Buffer(ByteArray* buf) : IoDevice(*new BufferImpl) {
  impl_->buffer_ = buf ? buf : &impl_->default_buffer_;
  impl_->default_buffer_.Clear();
}

Buffer::~Buffer() {}

ByteArray& Buffer::GetBuffer() {
  return *impl_->buffer_;
}

const ByteArray& Buffer::GetBuffer() const {
  return *impl_->buffer_;
}

void Buffer::SetBuffer(ByteArray* buf) {
  if (IsOpened()) {
    fun_log(Warning, "Buffer::setBuffer: Buffer is open");
    return;
  }

  if (buf) {
    impl_->buffer_ = buf;
  } else {
    impl_->buffer_ = &impl_->default_buffer_;
  }

  impl_->default_buffer_.Clear();
}

void Buffer::SetData(const ByteArray& data) {
  if (IsOpened()) {
    fun_log(Warning, "Buffer::setData: Buffer is open");
    return;
  }

  *impl_->buffer_ = data;
}

void Buffer::SetData(const char* data, int32 len) {
  SetData(ByteArray(data, len));
}

const ByteArray& Buffer::GetData() const {
  return *impl_->buffer_;
}

bool Buffer::Open(OpenMode open_mode) {
  if ((open_mode & (Append | Truncate)) != 0) {
    open_mode |= WriteOnly;
  }

  if ((open_mode & (ReadOnly | WriteOnly)) == 0) {
    fun_log(Warning, "Buffer::open: Buffer access not specified");
    return false;
  }

  if ((open_mode & Truncate) == Truncate) {
    impl_->buffer_->Resize(0);
  }

  return IoDevice::Open(open_mode | IoDevice::Unbuffered);
}

void Buffer::Close() {
  IoDevice::Close();
}

int64 Buffer::Size() const {
  return int64(impl_->buffer_->Count());
}

int64 Buffer::Tell() const {
  return IoDevice::Tell();
}

bool Buffer::Seek(int64 position) {
  if (position > impl_->buffer_->Count() && IsWritable()) {
    // 쓰기가 가능한 상태에서 커서가 데이터길이를 넘어서는 경우에는
    // 끝으로 이동 시키고, 추가 분에 대해서 기록해주도록 한다.
    // 단, 추가분은 0으로 채운다.
    if (Seek(impl_->buffer_->Count())) {
      const int64 over_len = position - impl_->buffer_->Count();
      if (Write(ByteArray(over_len, 0)) != over_len) {
        fun_log(Warning, "Buffer::Seek: Unable to fill over");
        return false;
      }
    } else {
      return false;
    }
  } else if (position > impl_->buffer_->Count() || position < 0) {
    fun_log(Warning, "Buffer::Seek: Invalid position: %d", int32(position));
    return false;
  }

  return IoDevice::Seek(position);
}

bool Buffer::AtEnd() const {
  return IoDevice::AtEnd();
}

bool Buffer::CanReadLine() const {
  if (!IsOpened()) {
    return false;
  }

  return impl_->buffer_->IndexOf('\n', int32(Tell())) != -1 || IoDevice::CanReadLine();
}

int64 Buffer::ReadData(char* buf, int64 max_len) {
  fun_check(buf || max_len == 0);
  fun_check(max_len >= 0);

  const int64 readable_len = MathBase::Min(max_len, int64(impl_->buffer_->Count()) - Tell());
  if (readable_len <= 0) {
    return 0;
  }

  UnsafeMemory::Memcpy(buf, impl_->buffer_->ConstData() + Tell(), readable_len);
  return readable_len;
}

int64 Buffer::WriteData(const char* data, int64 len) {
  fun_check(data || len == 0);
  fun_check(len >= 0);

  const int32 over_len = Tell() + len - impl_->buffer_->Count();
  if (over_len > 0) // overflow {
    const int32 new_len = impl_->buffer_->Count() + over_len;

    impl_->buffer_->Resize(new_len);

    if (impl_->buffer_->Count() != new_len) // could not resize {
      fun_log(Warning, "Buffer::writeData: Memory allocation error");
      return -1;
    }
  }

  UnsafeMemory::Memcpy(impl_->buffer_->MutableData() + Tell(), data, (size_t)len);
}

} // namespace fun
