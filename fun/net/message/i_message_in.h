#pragma once

#include "fun/net/message/message.h"

namespace fun {
namespace net {

class FUN_NET_API MessageInView {
 public:
  const uint8* offset;
  const uint8* end;

  MessageInView() : offset(nullptr), end(nullptr) {}

  MessageInView(const uint8* offset, const uint8* end)
    : offset(offset), end(end) {}

  MessageInView(const MessageInView&) = default;
  MessageInView& operator = (const MessageInView&) = default;
  MessageInView(MessageInView&&) = default;
  MessageInView& operator = (MessageInView&&) = default;
};


class FUN_NET_API IMessageIn {
 public:
  /**
   * Dummy virtual destructor for inheritance.
   */
  virtual ~IMessageIn() {}

  virtual bool ExceptionsEnabled() const = 0;
  virtual void SetExceptionsEnabled(bool enable) = 0;

  /**
   * Message data pointer.
   */
  virtual const uint8* ConstData() const = 0;

  /**
   * Returns the message length.
   * (If the read range is set, it returns the length set in the range.)
   */
  virtual int32 Length() const = 0;

  /**
   * Returns the current reading position.
   */
  virtual int32 Tell() const = 0;
  /**
   * Set the reading position.
   */
  virtual void Seek(int32 new_position) = 0;

  /**
   * The data pointer at the current read position.
   */
  virtual const uint8* ReadablePtr() const = 0;
  /**
   * Currently readable data length. That is, the length of the data that is still less read.
   */
  virtual int32 ReadableLength() const = 0;

  /**
   * Checks whether data can be read by the length specified in `length`.
   */
  FUN_ALWAYS_INLINE bool CanRead(int32 length) const { return ReadableLength() >= length; }
  /**
   * Check whether all data have been read.
   * Used to check whether the message has been read normally.
   */
  FUN_ALWAYS_INLINE bool AtEnd() const { return ReadableLength() <= 0; }
  FUN_ALWAYS_INLINE bool AtBegin() const { return Tell() == 0; }

  /**
   * Returns the maximum message length. Used internally to check data integrity etc.
   */
  virtual int32 MessageMaxLength() const = 0;
  /**
   * Set the maximum message length.
   */
  virtual void SetMessageMaxLength(int32 maximum_length) = 0;

  /**
   * Move the reading position to the beginning.
   * it should not be used if the reading position is limited.
   */
  FUN_ALWAYS_INLINE void SeekToBegin() { fun_check(!IsViewAdjusted()); Seek(0); }
  /**
   * Move the reading position to the end.
   * it should not be used if the reading position is limited.
   */
  FUN_ALWAYS_INLINE void SeekToEnd() { fun_check(!IsViewAdjusted()); Seek(Length()); }

  /**
   * Skip the reading position. You can only move forward, and you can not move backward.
   * 
   * \param amount - Distance in bytes to move. (Can not be negative.)
   */
  virtual bool SkipRead(int32 amount) = 0;

  /**
   * Read data from the message stream. Attempts to read as specified by `length`,
   * Returns the actual length read as a return value.
   * 
   * \param buffer - The buffer pointer from which to read the data.
   * \param length - The length of the data to read.
   * 
   * \return The length of the data actually read.
   */
  virtual int32 TryReadRawBytes(void* buffer, int32 length) = 0;

  /**
   * Read data from the message stream.
   * Unlike the TryReadRawBytes function, it should read as much as the length specified in `length`,
   * If the length specified by `length` can not be read,
   * `False`, and no data is filled in` buffer`.
   * 
   * \param buffer - The buffer pointer from which to read the data.
   * \param length - The length of the data to read.
   * 
   * \return Returns whether it has been read properly.
   */
  FUN_ALWAYS_INLINE bool ReadRawBytes(void* buffer, int32 length) {
    return TryReadRawBytes(buffer, length) == length;
  }

  virtual bool ReadFixed8(uint8& out_value) = 0;
  virtual bool ReadFixed16(uint16& out_value) = 0;
  virtual bool ReadFixed32(uint32& out_value) = 0;
  virtual bool ReadFixed64(uint64& out_value) = 0;

  virtual bool ReadVarint8(uint8& out_value) = 0;
  virtual bool ReadVarint16(uint16& out_value) = 0;
  virtual bool ReadVarint32(uint32& out_value) = 0;
  virtual bool ReadVarint64(uint64& out_value) = 0;

  virtual void SetRecursionLimit(int32 new_limit) = 0;
  virtual void IncreaseRecursionDepth() = 0;
  virtual void DecreaseRecursionDepth() = 0;

  virtual MessageInView PushView() = 0;
  virtual void PopView(const MessageInView& previous_view) = 0;
  virtual void AdjustView(int32 length) = 0;
  virtual bool IsViewAdjusted() const = 0;
};


class ScopedMessageInPositionTransaction {
 public:
  ScopedMessageInPositionTransaction(IMessageIn& message)
    : message_(message) {
    saved_read_position_ = message_.Tell();
  }

  ~ScopedMessageInPositionTransaction() {
    Rollback();
  }

  bool IsRollbackable() const {
    return saved_read_position_ != -1;
  }

  void Commit() {
    saved_read_position_ = -1;
  }

  void Rollback() {
    if (saved_read_position_ != -1) {
      message_.Seek(saved_read_position_);
      saved_read_position_ = -1;
    }
  }

  ScopedMessageInPositionTransaction() = delete;
  ScopedMessageInPositionTransaction(const ScopedMessageInPositionTransaction&) = delete;
  ScopedMessageInPositionTransaction& operator = (const ScopedMessageInPositionTransaction&) = delete;

 private:
  /** 메시지 객체입니다. */
  IMessageIn& message_;

  /**
   * 저장된 메시지의 읽기 위치입니다.  rollback시 이 이위치로 이동합니다.
   * -1인 경우에는 이미 commit된 상태입니다.
   */
  int32 saved_read_position_;
};


class ScopedMessageInExceptionEnable {
 public:
  ScopedMessageInExceptionEnable() {}

  FUN_ALWAYS_INLINE ScopedMessageInExceptionEnable(IMessageIn& input, bool enable = true)
    : input_(&input) {
    saved_exceptions_enabled_ = input_->ExceptionsEnabled();
    input_->SetExceptionsEnabled(enable);
  }

  FUN_ALWAYS_INLINE ~ScopedMessageInExceptionEnable() {
    input_->SetExceptionsEnabled(saved_exceptions_enabled_);
  }

 private:
  IMessageIn* input_;
  bool saved_exceptions_enabled_;
};


class ScopedMessageInRecursionGuard {
 public:
  FUN_ALWAYS_INLINE ScopedMessageInRecursionGuard(IMessageIn& input)
    : input_(&input) {
    input_->IncreaseRecursionDepth();
  }

  FUN_ALWAYS_INLINE ~ScopedMessageInRecursionGuard() {
    input_->DecreaseRecursionDepth();
  }

 private:
  IMessageIn* input_;
};


/**
 * Used to limit the locally readable range in the message stream.
 * Automatically restores the limit range to the previous range upon stack unwinding.
 */
class ScopedMessageInViewGuard {
 public:
  FUN_ALWAYS_INLINE ScopedMessageInViewGuard(IMessageIn& input)
    : input_(&input) {
    previous_view_ = input_->PushView();
  }

  FUN_ALWAYS_INLINE ~ScopedMessageInViewGuard() {
    input_->PopView(previous_view_);
  }

 private:
  IMessageIn* input_;
  MessageInView previous_view_;
};

#define FUN_SCOPED_MESSAGEIN_EXCEPTIONS_ENABLED(MessageIn) \
  ScopedMessageInExceptionsEnabled ScopedExceptionsEnabled(MessageIn, true);
#define SCOPED_MESSAGEIN_EXCEPTIONS_DISABLED(MessageIn) \
  ScopedMessageInExceptionsEnabled ScopedExceptionsDisabled(MessageIn, false);

#define FUN_SCOPED_MESSAGEIN_GUARDS(MessageIn) \
  ScopedMessageInRecursionGuard recursion_guard(MessageIn); \
  ScopedMessageInViewGuard view_guard(MessageIn);

} // namespace net
} // namespace fun
