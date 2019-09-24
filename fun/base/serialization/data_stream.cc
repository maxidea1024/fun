#include "fun/base/data_stream.h"

namespace fun {

#define FUN_VOID

#undef CHECK_STREAM_PRECOND

#ifndef FUN_NO_DEBUG

#define CHECK_STREAM_PRECOND(rv) \
  if (!device_) { \
    fun_log(Warning, "DataStream: No device"); \
    return rv; \
  }

#else

#define CHECK_STREAM_PRECOND(rv) \
  if (!device_) { \
    return rv; \
  }

#endif


#define CHECK_STREAM_WRITE_PRECOND(rv) \
  CHECK_STREAM_PRECOND(rv) \
  if (status_ != Status::Ok) { \
    return rv; \
  }

#define CHECK_STREAM_TRANSACTION_PRECOND(rv) \
  if (!d || d->transaction_depth_ == 0) { \
    fun_log(Warning, "DataStream: No transaction in progress"); \
    return rv; \
  }

DataStream::DataStream() {
  device_ = nullptr;
  own_device_ = false;
  byte_order_ = BigEndian;
  version_ = Qt_DefaultCompiledVersion;
  no_swap_ = SysInfo::ByteOrder == SysInfo::BigEndian;
  status_ = Status::Ok;
}

DataStream::DataStream(IoDevice* device) {
  device_ = device; // set device
  own_device_ = false;
  byte_order_ = BigEndian;  // default byte order
  version_ = Qt_DefaultCompiledVersion;
  no_swap_ = SysInfo::ByteOrder == SysInfo::BigEndian;
  status_ = Status::Ok;
}

DataStream::DataStream(ByteArray* ba, IoDevice::OpenMode open_mode) {
  fun_check_ptr(ba);

  IoBuffer* buf = new IoBuffer(ba);

//#ifndef QT_NO_QOBJECT
//  buf->BlockSignals(true);
//#endif

  buf->Open(open_mode);
  device_ = buf;
  own_device_ = true;

  byte_order_ = BigEndian;
  version_ = Qt_DefaultCompiledVersion;
  no_swap_ = SysInfo::ByteOrder == SysInfo::BigEndian;
  status_ = Status::Ok;
}

DataStream::DataStream(const ByteArray& ba) {
  IoBuffer* buf = new IoBuffer;

//#ifndef QT_NO_QOBJECT
//  buf->BlockSignals(true);
//#endif

  buf->SetData(ba);
  buf->Open(IoDevice::ReadOnly);
  device_ = buf;
  own_device_ = true;

  byte_order_ = BigEndian;
  version_ = Qt_DefaultCompiledVersion;
  no_swap_ = SysInfo::ByteOrder == SysInfo::BigEndian;
  status_ = Status::Ok;
}

DataStream::~DataStream() {
  if (own_device_) {
    delete device_;
  }
}

void DataStream::SetDevice(IoDevice* device) {
  if (own_device_) {
    delete device_;
    own_device_ = false;
  }

  device_ = device;
  //공유 플래그를 별도로 지정할수 있도록 할까??
  fun_check(own_device_ == false);
}

void DataStream::UnsetDevice() {
  SetDevice(nullptr);
}

bool DataStream::AtEnd() const {
  return device_ ? device_->AtEnd() : true;
}

//TODO 제거 대상.
DataStream::FloatingPointPrecision DataStream::GetFloatingPointPrecision() const {
  return d == 0 ? DataStream::DoublePrecision : d->floating_point_precision_;
}

//TODO 제거 대상.
void DataStream::SetFloatingPointPrecision(DataStream::FloatingPointPrecision precision) {
  if (d == 0) {
    d.Reset(new DataStreamImpl());
  }

  d->floating_point_precision_ = precision;
}

DataStream::Status DataStream::GetStatus() const {
  return status_;
}

void DataStream::ResetStatus() {
  status_ = Status::Ok;
}

void DataStream::SetStatus(Status status) {
  // 이미 에러가 난 상황에서는 ResetStatus()를 호출하기 전까지는 재설정하지 않음.
  if (status_ == Status::Ok) {
    status_ = status;
  }
}

void DataStream::SetByteOrder(ByteOrder byte_order) {
  byte_order_ = byte_order;

  if (SysInfo::ByteOrder == SysInfo::BigEndian) {
    no_swap_ = (byte_order_ == BigEndian);
  } else {
    no_swap_ = (byte_order_ == LittleEndian);
  }
}

void DataStream::StartTransaction() {
  CHECK_STREAM_PRECOND(FUN_VOID)

  if (d == 0) {
    d.Reset(new DataStreamImpl());
  }

  if (++d->transaction_depth_ == 1) {
    device_->StartTransaction();
    ResetStatus();
  }
}

bool DataStream::CommitTransaction() {
  CHECK_STREAM_TRANSACTION_PRECOND(false)

  if (--d->transaction_depth_ == 0) {
    CHECK_STREAM_PRECOND(false)

    if (status_ == Status::ReadPastEnd) {
      device_->RollbackTransaction();
      return false;
    }

    device_->CommitTransaction();
  }

  return status_ == Status::Ok;
}

void DataStream::RollbackTransaction() {
  SetStatus(Status::ReadPastEnd);

  CHECK_STREAM_TRANSACTION_PRECOND(FUN_VOID);
  if (--d->transaction_depth_ != 0) {
    return;
  }

  CHECK_STREAM_PRECOND(FUN_VOID)
  if (status_ == Status::ReadPastEnd) {
    device_->RollbackTransaction();
  } else {
    device_->CommitTransaction();
  }
}

void DataStream::AbortTransaction() {
  status_ = Status::ReadCorruptData;

  CHECK_STREAM_TRANSACTION_PRECOND(FUN_VOID);
  if (--d->transaction_depth_ != 0) {
    return;
  }

  CHECK_STREAM_PRECOND(FUN_VOID)
  device_->CommitTransaction();
}

int32 DataStream::ReadBlock(char* buf, int32 len) {
  // Disable reads on failure in transacted stream
  if (status_ != Status::Ok && device_->IsInTransaction()) {
    return -1;
  }

  const int32 readed_len = device_->Read(buf, len);
  if (readed_len != len) {
    SetStatus(Status::ReadPastEnd);
  }

  return readed_len;
}

DataStream& DataStream::operator >> (int8& i) {
  i = 0;
  CHECK_STREAM_PRECOND(*this);
  char c;
  if (ReadBlock(&c, 1) == 1) {
    i = int8(c);
  }

  return *this;
}

DataStream& DataStream::operator >> (int16& i) {
  i = 0;
  CHECK_STREAM_PRECOND(*this);
  if (ReadBlock(reinterpret_cast<char*>(&i), 2) != 2) {
    i = 0;
  } else {
    if (!no_swap_) {
      i = qbswap(i);
    }
  }

  return *this;
}

DataStream& DataStream::operator >> (int32& i) {
  i = 0;
  CHECK_STREAM_PRECOND(*this);
  if (ReadBlock(reinterpret_cast<char*>(&i), 4) != 4) {
    i = 0;
  } else {
    if (!no_swap_) {
      i = qbswap(i);
    }
  }

  return *this;
}

DataStream& DataStream::operator >> (int64& i) {
  i = int64(0);
  CHECK_STREAM_PRECOND(*this);
  if (GetVersion() < 6) {
    uint32 i1, i2;
    *this >> i2 >> i1;
    i = ((uint64)i1 << 32) + i2;
  } else {
    if (ReadBlock(reinterpret_cast<char*>(&i), 8) != 8) {
      i = int64(0);
    } else {
      if (!no_swap_) {
        i = qbswap(i);
      }
    }
  }

  return *this;
}

DataStream& DataStream::operator >> (bool& i) {
  int8 v;
  *this >> v;
  i = !!v;

  return *this;
}

DataStream& DataStream::operator >> (float& f) {
  if (GetVersion() >= DataStream::Qt_4_6
      && GetFloatingPointPrecision() == DataStream::DoublePrecision) {
    double d;
    *this >> d;
    f = d;
    return *this;
  }

  f = 0.0f;
  CHECK_STREAM_PRECOND(*this);

  if (ReadBlock(reinterpret_cast<char*>(&f), 4) != 4) {
    f = 0.0f;
  } else {
    if (!no_swap_) {
      union {
        float val1;
        uint32 val2;
      } x;
      x.val2 = qbswap(*reinterpret_cast<uint32*>(&f));
      f = x.val1;
    }
  }

  return *this;
}

DataStream& DataStream::operator >> (double& f) {
  if (GetVersion() >= DataStream::Qt_4_6
      && GetFloatingPointPrecision() == DataStream::SinglePrecision) {
    float d;
    *this >> d;
    f = d;
    return *this;
  }

  f = 0.0;
  CHECK_STREAM_PRECOND(*this);

  if (ReadBlock(reinterpret_cast<char*>(&f), 8) != 8) {
    f = 0.0;
  } else {
    if (!no_swap_) {
      union {
        double val1;
        uint64 val2;
      } x;
      x.val2 = qbswap(*reinterpret_cast<uint64*>(&f));
      f = x.val1;
    }
  }

  return *this;
}

DataStream& DataStream::operator >> (float16& f) {
  return *this >> reinterpret_cast<int16&>(f);
}

DataStream& DataStream::operator >> (char*& s) {
  uint32 len = 0;
  return ReadBytes(s, len);
}

DataStream& DataStream::ReadBytes(char*& s, uint32& l) {
  s = 0;
  l = 0;
  CHECK_STREAM_PRECOND(*this)

  uint32 len;
  *this >> len;
  if (len == 0) {
    return *this;
  }

  const uint32 Step = 1024 * 1024;
  uint32 allocated = 0;
  char* prev_buf = 0;
  char* cur_buf = 0;

  do {
    int32 block_size = MathBase::Min(Step, len - allocated);
    prev_buf = cur_buf;
    cur_buf = new char[allocated + block_size + 1];
    if (prev_buf) {
      UnsafeMemory::Memcpy(cur_buf, prev_buf, allocated);
      delete[] prev_buf;
    }

    if (ReadBlock(cur_buf + allocated, block_size) != block_size) {
      delete[] cur_buf;
      return *this;
    }
    allocated += block_size;
  } while (allocated < len);

  s = cur_buf;
  s[len] = '\0';
  l = (uint32)len;
  return *this;
}

int32 DataStream::ReadRawData(char* buf, int32 len) {
  CHECK_STREAM_PRECOND(-1)

  return ReadBlock(buf, len);
}

DataStream& DataStream::operator << (int8 i) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  if (!device_->PutChar(i)) {
    status_ = Status::WriteFailed;
  }

  return *this;
}

DataStream& DataStream::operator << (int16 i) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  if (!no_swap_) {
    i = qbswap(i);
  }

  if (device_->Write((char*)&i, sizeof(int16)) != sizeof(int16)) {
    status_ = Status::WriteFailed;
  }

  return *this;
}

DataStream& DataStream::operator << (int32 i) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  if (!no_swap_) {
    i = qbswap(i);
  }

  if (device_->Write((char*)&i, sizeof(int32)) != sizeof(int32)) {
    status_ = Status::WriteFailed;
  }

  return *this;
}

DataStream& DataStream::operator << (int64 i) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  if (GetVersion() < 6) {
    uint32 i1 = i & 0xffffffff;
    uint32 i2 = i >> 32;
    *this << i2 << i1;
  } else {
    if (!no_swap_) {
      i = qbswap(i);
    }

    if (device_->Write((char*)&i, sizeof(int64)) != sizeof(int64)) {
      status_ = Status::WriteFailed;
    }
  }

  return *this;
}

DataStream& DataStream::operator << (bool i) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  if (!device_->PutChar(int8(i))) {
    status_ = Status::WriteFailed;
  }

  return *this;
}

DataStream& DataStream::operator << (float f) {
  if (GetVersion() >= DataStream::Qt_4_6
      && GetFloatingPointPrecision() == DataStream::DoublePrecision) {
    *this << double(f);
    return *this;
  }

  CHECK_STREAM_WRITE_PRECOND(*this);

  float g = f; // fixes float-on-stack problem
  if (!no_swap_) {
    union {
      float val1;
      uint32 val2;
    } x;
    x.val1 = g;
    x.val2 = qbswap(x.val2);

    if (device_->Write((char*)&x.val2, sizeof(float)) != sizeof(float)) {
      status_ = Status::WriteFailed;
    }
    return *this;
  }

  if (device_->Write((char*)&g, sizeof(float)) != sizeof(float)) {
    status_ = Status::WriteFailed;
  }

  return *this;
}

DataStream& DataStream::operator << (double f) {
  if (GetVersion() >= DataStream::Qt_4_6
      && GetFloatingPointPrecision() == DataStream::SinglePrecision) {
    *this << float(f);
    return *this;
  }

  CHECK_STREAM_WRITE_PRECOND(*this);

  if (no_swap_) {
    if (device_->Write((char*)&f, sizeof(double)) != sizeof(double)) {
      status_ = Status::WriteFailed;
    }
  } else {
    union {
      double val1;
      uint64 val2;
    } x;
    x.val1 = f;
    x.val2 = qbswap(x.val2);
    if (device_->Write((char*)&x.val2, sizeof(double)) != sizeof(double)) {
      status_ = Status::WriteFailed;
    }
  }

  return *this;
}

DataStream& DataStream::operator << (float16 f) {
  return *this << reinterpret_cast<int16&>(f);
}

DataStream& DataStream::operator << (const char* s) {
  if (!s) {
    *this << (uint32)0;
    return *this;
  }

  //TODO nul-term
  uint32 len = qstrlen(s) + 1; // also Write null terminator
  *this << (uint32)len; // Write length specifier
  WriteRawData(s, len);

  return *this;
}

DataStream& DataStream::WriteBytes(const char* data, uint32 len) {
  CHECK_STREAM_WRITE_PRECOND(*this);

  //WriteCounter(len);
  *this << (uint32)len; // Write length specifier
  if (len) {
    WriteRawData(data, len);
  }

  return *this;
}

int32 DataStream::WriteRawData(const char* data, int32 len) {
  CHECK_STREAM_WRITE_PRECOND(-1);

  const int32 written_len = device_->Write(data, len);
  if (written_len != len) {
    //Truncated?
    status_ = Status::WriteFailed;
  }

  return written_len;
}

int32 DataStream::SkipRawData(int32 len) {
  CHECK_STREAM_PRECOND(-1);

  if (status_ != Status::Ok && device_->IsInTransaction()) {
    return -1;
  }

  const int32 skipped_len = device_->Skip(len);
  if (skipped_len != len) {
    SetStatus(ReadPastEnd);
  }

  return skipped_len;
}

} // namespace fun
