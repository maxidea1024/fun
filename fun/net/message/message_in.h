#pragma once

#include "fun/net/message/imessage_in.h"

namespace fun {
namespace net {

class MessageOut;

class FUN_NET_API MessageIn : public IMessageIn {
 public:
  MessageIn();
  MessageIn(const MessageOut& source);
  MessageIn(const MessageOut& source, int32 offset, int32 length);
  MessageIn(const MessageIn& source, bool with_source_position = true);
  MessageIn(const MessageIn& source, int32 offset, int32 length);
  MessageIn(const ByteArray& source);
  MessageIn(const ByteArray& source, int32 offset, int32 length);
  MessageIn& operator=(const MessageIn& rhs);

  static MessageIn From(const MessageOut& source);
  static MessageIn From(const MessageOut& source, int32 offset, int32 length);
  static MessageIn From(const MessageIn& source,
                        bool with_source_position = true);
  static MessageIn From(const MessageIn& source, int32 offset, int32 length);
  static MessageIn From(const ByteArray& source);
  static MessageIn From(const ByteArray& source, int32 offset, int32 length);

  /**
   * 읽기 위치 고려없이, 전체 데이터를 ByteArray 형태로 복사한 것을 돌려줍니다.
   * 특별한 경우가 아니라면 사용하지 않는것이 좋습니다. 복사 비용이 뒤따릅니다.
   */
  ByteArray ToAllBytesCopy() const;

  /**
   * 읽기 위치 고려없이, 전체 데이터를 MessageIn 형태로 참조하여 돌려줍니다.
   */
  MessageIn ToAllMessage() const;

  /**
   * 읽기 위치 고려없이, 전체 데이터를 raw 형태로 돌려줍니다. 원본 객체가
   * 파괴되면 반환된 객체는 더이상 안전하지 않습니다.
   */
  ByteArray ToAllBytesRaw() const;

  /**
   * 읽기 위치 고려없이, 전체 데이터를 ByteArrayView 형태로 돌려줍니다. 원본
   * 객체가 파괴되면 반환된 객체는 더이상 안전하지 않습니다.
   */
  ByteArrayView ToAllBytesView() const;

  /**
   * 읽기 가능한 데이터를 ByteArray 형태로 복사해서 돌려줍니다. MessageIn 객체가
   * 파괴되는 경우, 불가피하게 데이터 사본이 필요한 경우에 사용합니다.
   * 가급적이면, 사용하지 않는것이 좋습니다.
   */
  ByteArray ToReadableBytesCopy() const;

  /**
   * 읽기 가능한 데이터를 복사 없이 참조를 유지한채 접근할 수 있도록 만든,
   * MessageIn 객체를 만들어 반환합니다. 내부적으로 버퍼는 공유되며, 이 함수가
   * 반환한 객체는 원래 참조되던 내부버퍼가 다른곳에서 수정된다고 하여도 내용이
   * 변경되지 않습니다. 이러한 이유는 내부버퍼가 COW(Copy On Write) 형태로
   * 구현되어 있기 때문입니다.
   */
  MessageIn ToReadableMessage() const;

  /**
   * 읽기 가능한 데이터를 Raw형태의 ByteArray 형태로 돌려줍니다. 원본 객체가
   * 파괴될 경우에 더이상 안전하지 않습니다.  바로 사용하고 회수되는 경우에
   * 한해서 사용하는 것이 좋습니다.
   */
  ByteArray ToReadableBytesRaw() const;

  /**
   * 읽기 가능한 데이터를 ByteArrayView 형태로 돌려줍니다.
   * ByteArrayView 객체 자체가 원본 객체가 유효한 경우를 가정하므로, 원본 객체가
   * 파괴되면 메모리 오류가 발생하게 됩니다.
   */
  ByteArrayView ToReadableBytesView() const;

  /**
   * 내부 버퍼를 리셋합니다.
   */
  void ResetBuffer();

  bool ReadAsShared(MessageIn& to, int32 length_to_read);  // share(no copy)
  bool ReadAsCopy(ByteArray& to, int32 length_to_read);    // copy
  bool ReadAsRaw(ByteArray& to, int32 length_to_read);     // raw(no copy)

  // IMessageIn interface
  bool ExceptionsEnabled() const override;
  void SetExceptionsEnabled(bool enable) override;

  const uint8* ConstData() const override;

  int32 Length() const override;

  int32 Tell() const override;
  void Seek(int32 new_position) override;

  const uint8* ReadablePtr() const override;
  int32 ReadableLength() const override;

  int32 MessageMaxLength() const override;
  void SetMessageMaxLength(int32 maximum_length) override;

  bool SkipRead(int32 amount) override;

  int32 TryReadRawBytes(void* buffer, int32 length) override;

  bool ReadFixed8(uint8& out_value) override;
  bool ReadFixed16(uint16& out_value) override;
  bool ReadFixed32(uint32& out_value) override;
  bool ReadFixed64(uint64& out_value) override;

  bool ReadVarint8(uint8& out_value) override;
  bool ReadVarint16(uint16& out_value) override;
  bool ReadVarint32(uint32& out_value) override;
  bool ReadVarint64(uint64& out_value) override;

  void SetRecursionLimit(int32 new_limit) override;
  void IncreaseRecursionDepth() override;
  void DecreaseRecursionDepth() override;

  MessageInView PushView() override;
  void PopView(const MessageInView& previous_view) override;
  void AdjustView(int32 length) override;
  bool IsViewAdjusted() const override;

  int32 GetMasterViewOffset() const;
  int32 GetMasterViewLength() const;

  bool IsDetached() const;
  void Detach();

  void CheckConsistency() const;

  void EnsureAtOrigin();

 private:
  bool ReadVarint8Fallback(uint8& out_value);
  bool ReadVarint16Fallback(uint16& out_value);
  bool ReadVarint32Fallback(uint32& out_value);
  bool ReadVarint64Fallback(uint64& out_value);

  bool ReadVarint8Slow(uint8& out_value);
  bool ReadVarint16Slow(uint16& out_value);
  bool ReadVarint32Slow(uint32& out_value);
  bool ReadVarint64Slow(uint64& out_value);

  /** shareable buffer. */
  ByteArray sharable_buffer_;

  /** 메인 뷰를 설정합니다. */
  void SetupMasterView();

  /** 메인 뷰를 설정합니다. */
  void SetupMasterView(int32 master_view_offset, int32 master_view_length,
                       int32 source_length = -1);

  bool exceptions_enabled_;

  /** current read pointer */
  const uint8* buffer_;
  /** Start pointer of buffer */
  const uint8* buffer_start_;
  /** End pointer of buffer */
  const uint8* buffer_end_;

  /** original start pointer of buffer */
  const uint8* original_buffer_;
  /** original end pointer of buffer */
  const uint8* original_buffer_end_;

  int32 message_max_length_;

  /** maximum recursion limit */
  int32 recursion_limit_;
  /** current recursion depth */
  int32 recursion_depth_;

  bool Advance(int32 amount);

  static const uint8* ReadVarint8FromBuffer(const uint8* buffer,
                                            uint8& out_value);
  static const uint8* ReadVarint16FromBuffer(const uint8* buffer,
                                             uint16& out_value);
  static const uint8* ReadVarint32FromBuffer(const uint8* buffer,
                                             uint32& out_value);
  static const uint8* ReadVarint64FromBuffer(const uint8* buffer,
                                             uint64& out_value);
};

//
// inlines
//

FUN_ALWAYS_INLINE bool MessageIn::ExceptionsEnabled() const {
  return exceptions_enabled_;
}

FUN_ALWAYS_INLINE void MessageIn::SetExceptionsEnabled(bool enable) {
  exceptions_enabled_ = enable;
}

FUN_ALWAYS_INLINE int32 MessageIn::MessageMaxLength() const {
  return message_max_length_;
}

FUN_ALWAYS_INLINE void MessageIn::SetMessageMaxLength(int32 maximum_length) {
  message_max_length_ = maximum_length;
}

FUN_ALWAYS_INLINE int32 MessageIn::ReadableLength() const {
  return int32(buffer_end_ - buffer_);
}

FUN_ALWAYS_INLINE const uint8* MessageIn::ReadablePtr() const {
  return buffer_;
}

FUN_ALWAYS_INLINE ByteArray MessageIn::ToAllBytesCopy() const {
  return ByteArray((const char*)ConstData(), Length());  // copy
}

FUN_ALWAYS_INLINE MessageIn MessageIn::ToAllMessage() const {
  return From(*this, false);  // without source posiiton
}

FUN_ALWAYS_INLINE ByteArray MessageIn::ToAllBytesRaw() const {
  return ByteArray::FromRawData((const char*)ConstData(),
                                Length());  // raw(unsafe)
}

FUN_ALWAYS_INLINE ByteArrayView MessageIn::ToAllBytesView() const {
  return ByteArrayView((const char*)ConstData(), Length());  // reference
}

FUN_ALWAYS_INLINE ByteArray MessageIn::ToReadableBytesCopy() const {
  return ByteArray((const char*)ReadablePtr(), ReadableLength());  // copy
}

FUN_ALWAYS_INLINE MessageIn MessageIn::ToReadableMessage() const {
  return MessageIn(sharable_buffer_, Tell(), ReadableLength());  // share
}

FUN_ALWAYS_INLINE ByteArray MessageIn::ToReadableBytesRaw() const {
  return ByteArray::FromRawData((const char*)ReadablePtr(),
                                ReadableLength());  // raw(unsafe)
}

FUN_ALWAYS_INLINE ByteArrayView MessageIn::ToReadableBytesView() const {
  return ByteArrayView((const char*)ReadablePtr(),
                       ReadableLength());  // reference
}

FUN_ALWAYS_INLINE int32 MessageIn::Tell() const {
  return int32(buffer_ - original_buffer_);
}

FUN_ALWAYS_INLINE int32 MessageIn::Length() const {
  // return int32(original_buffer_end_ - original_buffer_);
  return int32(buffer_end_ - buffer_start_);
}

FUN_ALWAYS_INLINE const uint8* MessageIn::ConstData() const {
  return original_buffer_;
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const MessageOut& source) {
  return MessageIn(source);
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const MessageOut& source,
                                            int32 offset, int32 length) {
  return MessageIn(source, offset, length);
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const MessageIn& source,
                                            bool with_source_position) {
  return MessageIn(source, with_source_position);
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const MessageIn& source,
                                            int32 offset, int32 length) {
  return MessageIn(source, offset, length);
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const ByteArray& source) {
  return MessageIn(source);
}

FUN_ALWAYS_INLINE MessageIn MessageIn::From(const ByteArray& source,
                                            int32 offset, int32 length) {
  return MessageIn(source, offset, length);
}

}  // namespace net
