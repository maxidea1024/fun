#pragma once

#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API DataStream {
 public:
  enum ByteOrder {
    BigEndian = SysInfo::BigEndian,
    LittleEndian = SysInfo::LittleEndian,
  };

  enum Status {
    Ok,
    ReadPastEnd,
    ReadCorruptData,
    WriteFailed,
  };

  // TODO 제거하자..
  enum FloatingPointPrecision {
    SinglePrecision,
    DoublePrecision,
  };

  DataStream();
  explicit DataStream(IoDevice* device);
  DataStream(ByteArray* buf, IoDevice::OpenMode flags);
  DataStream(const ByteArray& data);
  virtual ~DataStream();

  IoDevice* GetDevice() const;
  void SetDevice(IoDevice* device);
  void UnsetDevice();

  bool AtEnd() const;

  Status GetStatus() const;
  void SetStatus(Status status);
  void ResetStatus();

  // TODO 이것은 제거하는쪽으로...
  FloatingPointPrecision GetFloatingPointPrecision() const;
  void SetFloatingPointPrecision(FloatingPointPrecision precision);

  ByteOrder GetByteOrder() const;
  void SetByteOrder(ByteOrder byte_order);

  int32 GetVersion() const;
  void SetVersion(int32 version);

  DataStream& operator>>(bool&);
  DataStream& operator>>(int8&);
  DataStream& operator>>(int16&);
  DataStream& operator>>(int32&);
  DataStream& operator>>(int64&);
  DataStream& operator>>(uint8&);
  DataStream& operator>>(uint16&);
  DataStream& operator>>(uint32&);
  DataStream& operator>>(uint64&);
  DataStream& operator>>(float&);
  DataStream& operator>>(double&);
  DataStream& operator>>(std::nullptr_t& ptr) {
    ptr = nullptr;
    return *this;
  }
  DataStream& operator>>(char*&);

  DataStream& operator<<(bool);
  DataStream& operator<<(int8);
  DataStream& operator<<(int16);
  DataStream& operator<<(int32);
  DataStream& operator<<(int64);
  DataStream& operator<<(uint8);
  DataStream& operator<<(uint16);
  DataStream& operator<<(uint32);
  DataStream& operator<<(uint64);
  DataStream& operator<<(float);
  DataStream& operator<<(double);
  DataStream& operator<<(std::nullptr_t ptr) { return *this; }
  DataStream& operator<<(const char*);

  DataStream& ReadBytes(char*& buf, uint32& len);
  int32 ReadRawData(char* buf, int32 len);

  DataStream& WriteBytes(const char* data, uint32 len);
  int32 WriteRawData(const char* data, int32 len);

  int32 SkipRawData(int32 len);

  void StartTransaction();
  bool CommitTransaction();
  void RollbackTransaction();
  void AbortTransaction();

  DataStream(const DataStream&) = delete;
  DataStream& operator=(const DataStream&) = delete;

 private:
  // TODO 제거하자..
  ScopedPtr<DataStreamImpl> impl_;

  IoDevice* device_;
  bool own_device_;
  bool no_swap_;
  ByteOrder byte_order_;
  int32 version_;
  Status status_;
  // int3 transaction_depth_;

  int32 ReadBlock(char* buf, int32 len);
  friend class DataStream_internal::StreamStateSaver;
};

namespace DataStream_internal {

class StreamStateSaver {
 public:
  StreamStateSaver(DataStream* stream)
      : stream_(stream), old_status_(stream->GetStatus()) {
    if (!stream_->device_ || !stream_->device_->IsInTransaction()) {
      stream_->ResetStatus();
    }
  }

  ~StreamStateSaver() {
    if (old_status_ != DataStream::Ok) {
      stream_->ResetStatus();
      stream_->SetStatus(old_status_);
    }
  }

 private:
  DataStream* stream_;
  DataStream::Status old_status_;
};

// TODO 여기서 하는것 보다는 각 container 클래스에서 하는게 자연스러울듯...
template <typename Container>
DataStream& ReadyArrayFamilyContainer(DataStream& stream, Container& array) {
  StreamStateSaver scoped_state_saver(&stream);

  uint32 n;
  stream >> n;
  array.Clear(n);
  for (uint32 i = 0; i < n; ++i) {
    typename Container::ValueType tmp;
    stream >> tmp;
    if (stream.GetStatus() != DataStream::Ok) {
      array.Clear();
      break;
    }
    array.Append(tmp);
  }

  return stream;
}

template <typename Container>
DataStream& ReadListFamilyContainer(DataStream& s, Container& c) {
  StreamStateSaver scoped_state_saver(&s);

  c.Clear();
  uint32 n;
  s >> n;
  for (uint32 i = 0; i < n; ++i) {
    typename Container::value_type t;
    s >> t;
    if (s.GetStatus() != DataStream::Ok) {
      c.Clear();
      break;
    }
    c << t;
  }

  return s;
}

template <typename Container>
DataStream& ReadAssociativeContainer(DataStream& s, Container& c) {
  StreamStateSaver scoped_state_saver(&s);

  c.Clear();
  uint32 n;
  s >> n;
  for (uint32 i = 0; i < n; ++i) {
    typename Container::key_type k;
    typename Container::mapped_type t;
    s >> k >> t;
    if (s.GetStatus() != DataStream::Ok) {
      c.Clear();
      break;
    }
    c.insertMulti(k, t);
  }

  return s;
}

template <typename Container>
DataStream& WriteSequentialContainer(DataStream& s, const Container& c) {
  s << uint32(c.Count());
  for (const typename Container::value_type& t : c) {
    s << t;
  }

  return s;
}

template <typename Container>
DataStream& WriteAssociativeContainer(DataStream& s, const Container& c) {
  s << uint32(c.Count());
  // Deserialization should occur in the reverse order.
  // Otherwise, value() will return the least recently inserted
  // value instead of the most recently inserted one.
  auto it = c.constEnd();
  auto begin = c.constBegin();
  while (it != begin) {
    --it;
    s << it.key() << it.value();
  }

  return s;
}

}  // namespace DataStream_internal

//
// inlines
//

inline IoDevice* DataStream::GetDevice() const { return device_; }

inline DataStream::ByteOrder DataStream::GetByteOrder() const {
  return byte_order_;
}

inline int32 DataStream::GetVersion() const { return version_; }

inline void DataStream::SetVersion(int32 version) { version_ = version; }

inline DataStream& DataStream::operator>>(uint8& i) {
  return *this >> reinterpret_cast<int8&>(i);
}

inline DataStream& DataStream::operator>>(uint16& i) {
  return *this >> reinterpret_cast<int16&>(i);
}

inline DataStream& DataStream::operator>>(uint32& i) {
  return *this >> reinterpret_cast<int32&>(i);
}

inline DataStream& DataStream::operator>>(uint64& i) {
  return *this >> reinterpret_cast<int64&>(i);
}

inline DataStream& DataStream::operator<<(uint8 i) { return *this << int8(i); }

inline DataStream& DataStream::operator<<(uint16 i) {
  return *this << int16(i);
}

inline DataStream& DataStream::operator<<(uint32 i) {
  return *this << int32(i);
}

inline DataStream& DataStream::operator<<(uint64 i) {
  return *this << int64(i);
}

template <typename Enum>
inline DataStream& operator<<(DataStream& stream, Flags<Enum> e) {
  return stream << e.i;
}

template <typename Enum>
inline DataStream& operator>>(DataStream& stream, Flags<Enum>& e) {
  return stream >> e.i;
}

template <typename T>
inline DataStream& operator>>(DataStream& stream, List<T>> &l) {
  return DataStream_internal::ReadArrayFamilyContainer(stream, l);
}

template <typename T>
inline DataStream& operator<<(DataStream& stream, const List<T>> &l) {
  return DataStream_internal::WriteSequentialContainer(stream, l);
}

template <typename T>
inline DataStream& operator>>(DataStream& stream, LinkedList<T>> &l) {
  return DataStream_internal::ReadListFamilyContainer(stream, l);
}

template <typename T>
inline DataStream& operator<<(DataStream& stream, const LinkedList<T>> &l) {
  return DataStream_internal::WriteSequentialContainer(stream, l);
}

template <typename T>
inline DataStream& operator>>(DataStream& stream, Array<T>& a) {
  return DataStream_internal::ReadArrayFamilyContainer(stream, a);
}

template <typename T>
inline DataStream& operator<<(DataStream& stream, const Array<T>& a) {
  return DataStream_internal::WriteSequentialContainer(stream, a);
}

template <typename T>
inline DataStream& operator>>(DataStream& stream, Set<T>& s) {
  return DataStream_internal::ReadListFamilyContainer(stream, s);
}

template <typename T>
inline DataStream& operator<<(DataStream& stream, const Set<T>& s) {
  return DataStream_internal::WriteSequentialContainer(stream, s);
}

template <typename Key, typename T>
inline DataStream& operator>>(DataStream& stream, Hash<Key, T>& h) {
  return DataStream_internal::ReadAssociativeContainer(stream, h);
}

template <typename Key, typename T>
inline DataStream& operator<<(DataStream& stream, const Hash<Key, T>& h) {
  return DataStream_internal::WriteAssociativeContainer(stream, h);
}

template <typename Key, typename T>
inline DataStream& operator>>(DataStream& stream, Map<Key, T>& m) {
  return DataStream_internal::ReadAssociativeContainer(stream, m);
}

template <typename Key, typename T>
inline DataStream& operator<<(DataStream& stream, const Map<Key, T>& m) {
  return DataStream_internal::WriteAssociativeContainer(stream, m);
}

template <typename T1, typename T2>
inline DataStream& operator>>(DataStream& stream, Pair<T1, T2>& p) {
  stream >> p.first >> p.second;
  return stream;
}

template <typename T1, typename T2>
inline DataStream& operator<<(DataStream& stream, const Pair<T1, T2>& p) {
  stream << p.first << p.second;
  return stream;
}

}  // namespace fun
