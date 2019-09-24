#pragma once

#include "fun/net/net.h"
#include "fun/net/message/net_message.h"

namespace fun {
namespace net {

//class ArraySegment
//class ArraySegmentList

//TODO raw포인터가 아닌 ByteArray 객체를 담는 형태로 변경해, 복사를 줄이도록 하는게 좋을듯함.

class SendFragRefs {
 public:
  struct Frag {
    const uint8* data;
    int32 len;

    Frag() : data(nullptr), len(0) {}
    Frag(const uint8* data, int32 len) : data(data), len(len) {}
  };

 private:
  Array<Frag, InlineAllocator<32>> fragments_;
  int32 total_len_;

 public:
  int32 GetTotalLength() const { return total_len_; }
  bool IsEmpty() const { return total_len_ == 0; }

  SendFragRefs() : total_len_(0) {}

  SendFragRefs(const SendFragRefs& rhs)
    : total_len_(rhs.total_len_),
      fragments_(rhs.fragments_) {}

  SendFragRefs(const IMessageOut& msg) : total_len_(0) {
    Add(msg);
  }

  SendFragRefs(const IMessageIn& msg) : total_len_(0) {
    Add(msg);
  }

  SendFragRefs& operator = (const SendFragRefs& rhs) {
    fragments_ = rhs.fragments_;
    total_len_ = rhs.total_len_;
    return *this;
  }

  inline SendFragRefs(SendFragRefs&& rhs)
    : fragments_(MoveTemp(rhs.fragments_))
    , total_len_(rhs.total_len_) {
    rhs.total_len_ = 0;
  }

  inline SendFragRefs& operator = (SendFragRefs&& rhs) {
    fragments_ = MoveTemp(rhs.fragments_);
    total_len_ = rhs.total_len_;
    rhs.total_len_ = 0;
    return *this;
  }

  inline const Frag* ConstData() const {
    return fragments_.ConstData();
  }

  inline Frag& operator[](int32 index) {
    return fragments_[index];
  }

  inline const Frag& operator[](int32 index) const {
    return fragments_[index];
  }

  inline int32 Count() const {
    return fragments_.Count();
  }

  //TODO 이게 실제로 사용되는지??
  inline void Resize(int32 count) {
    //fragments_.ResizeUninitialized(Count);
    fragments_.ResizeZeroed(count);
  }

  inline void Add(const Frag& frag) {
    fragments_.Add(frag);
    total_len_ += frag.len;
  }

  inline void Add(const uint8* frag, int32 len) {
    Add(Frag(frag, len));
  }

  inline void Add(const ByteArray& frag) {
    Add(Frag((const uint8*)frag.ConstData(), frag.Len()));
  }

  inline void Add(const IMessageOut& msg) {
    Add(Frag(msg.ConstData(), msg.GetLength()));
  }

  // 읽기 가능한 데이터만 추가함. (전체를 추가하는게 아님. 좀 헷갈리려나??)
  inline void Add(const IMessageIn& msg) {
    Add(Frag(msg.GetReadableData(), msg.GetReadableLength()));
  }

  inline void Add(const SendFragRefs& other) {
    fragments_.Append(other.fragments_);
    total_len_ += other.total_len_;
  }

  inline SendFragRefs& operator << (const Frag& frag) { Add(frag); return *this; }
  inline SendFragRefs& operator << (const ByteArray& frag) { Add(frag); return *this; }
  inline SendFragRefs& operator << (const IMessageOut& frag) { Add(frag); return *this; }
  inline SendFragRefs& operator << (const IMessageIn& frag) { Add(frag); return *this; }
  inline SendFragRefs& operator << (const SendFragRefs& frag) { Add(frag); return *this; }

  void Insert(int32 position, const SendFragRefs& other) {
    fragments_.Insert(other.fragments_, position);
    total_len_ += other.total_len_;
  }

  void Insert(int32 position, const IMessageOut& msg) {
    Insert(position, Frag(msg.ConstData(), msg.Length()));
  }

  void Insert(int32 position, const IMessageIn& msg) {
    Insert(position, Frag(msg.ReadableData(), msg.ReadableLength()));
  }

  void Insert(int32 position, const ByteArray& frag) {
    Insert(position, Frag((const uint8*)frag.ConstData(), frag.Len()));
  }

  void Insert(int32 position, const Frag& frag) {
    fragments_.Insert(frag, position);
    total_len_ += frag.len;
  }

  void Insert(int32 position, const uint8* frag, int32 len) {
    Insert(position, Frag(frag, len));
  }

  void Clear() {
    fragments_.Reset(); // keep capacity
    total_len_ = 0;
  }

  //copy
  inline ByteArray ToBytes() const {
    ByteArray bytes(total_len_, NoInit);
    uint8* dst = (uint8*)bytes.MutableData();
    uint8* dst_ptr = dst;
    const int32 src_count = fragments_.Count();
    const Frag* src = fragments_.ConstData();
    for (int32 i = 0; i < src_count; ++i) {
      UnsafeMemory::Memcpy(dst_ptr, src->data, src->len);
      dst_ptr += src->len;
      ++src;
    }
    fun_check(int32(dst_ptr - dst) == total_len_);
    return bytes;
  }

  inline void CopyTo(Array<uint8>& To) const {
    To.ResizeUninitialized(total_len_);
    uint8* dst = To.MutableData();
    uint8* dst_ptr = dst;
    const int32 src_count = fragments_.Count();
    const Frag* src = fragments_.ConstData();
    for (int32 i = 0; i < src_count; ++i) {
      UnsafeMemory::Memcpy(dst_ptr, src->data, src->len);
      dst_ptr += src->len;
      ++src;
    }
    fun_check(int32(dst_ptr - dst) == total_len_);
  }

  //copy
  inline MessageOut ToMessageOut() const {
    return MessageOut(ToBytes());
  }
};

} // namespace net
} // namespace fun
