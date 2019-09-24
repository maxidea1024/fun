#pragma once

#include <ostream>
#include "fun/base/base.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

namespace Flags_internal {

template <size_t Size>
struct TypeStorage;

template <>
struct TypeStorage<sizeof(uint8)> {
  using Type = uint8;
};
template <>
struct TypeStorage<sizeof(uint16)> {
  using Type = uint16;
};
template <>
struct TypeStorage<sizeof(uint32)> {
  using Type = uint32;
};
template <>
struct TypeStorage<sizeof(uint64)> {
  using Type = uint64;
};

template <typename T>
struct UnderlyingType {
  using Type = typename TypeStorage<sizeof(T)>::Type;
};

template <typename Enum>
struct HasNone {
  template <typename U>
  static auto Test(U*) -> decltype(U::None, TrueType());
  template <typename U>
  static auto Test(...) -> FalseType;

  static constexpr bool Value =
      IsSame<decltype(Test<Enum>(0)), TrueType>::Value;
};

}  // namespace Flags_internal

template <typename T>
class Flags {
 public:
  using UnderlyingType = typename Flags_internal::UnderlyingType<T>::Type;

  static_assert(IsEnum<T>::Value, "Flags only works with enumerations");

  // TODO 이게 꼭 필요할까???
  // static_assert(Flags_internal::HasNone<T>::Value, "The enumeration needs a
  // None value"); static_assert(static_cast<UnderlyingType>(T::None) == 0, "None
  // should be 0");

  FUN_ALWAYS_INLINE Flags() : value_((T)0) {}

  FUN_ALWAYS_INLINE Flags(T value) : value_(value) {}

  // FUN_ALWAYS_INLINE Flags(UnderlyingType value) : value_((T)value) {}

#define __DEFINE_FLAGS_BITWISE_OP_CONST(op)                                   \
  FUN_ALWAYS_INLINE Flags<T> operator op(T rhs) const {                       \
    return Flags<T>(static_cast<T>(static_cast<UnderlyingType>(value_)        \
                                       op static_cast<UnderlyingType>(rhs))); \
  }                                                                           \
  FUN_ALWAYS_INLINE Flags<T> operator op(Flags<T> rhs) const {                \
    return Flags<T>(static_cast<T>(static_cast<UnderlyingType>(               \
        value_) op static_cast<UnderlyingType>(rhs.value_)));                 \
  }

  __DEFINE_FLAGS_BITWISE_OP_CONST(|)
  __DEFINE_FLAGS_BITWISE_OP_CONST(&)
  __DEFINE_FLAGS_BITWISE_OP_CONST (^)
  //__DEFINE_FLAGS_BITWISE_OP_CONST(~)
#undef __DEFINE_FLAGS_BITWISE_OP_CONST

#define __DEFINE_FLAGS_BITWISE_OP(op)                                        \
  FUN_ALWAYS_INLINE Flags<T>& operator op##=(T rhs) {                        \
    value_ = static_cast<T>(static_cast<UnderlyingType>(value_)              \
                                op static_cast<UnderlyingType>(rhs));        \
    return *this;                                                            \
  }                                                                          \
  FUN_ALWAYS_INLINE Flags<T>& operator op##=(Flags<T> rhs) {                 \
    value_ = static_cast<T>(static_cast<UnderlyingType>(value_)              \
                                op static_cast<UnderlyingType>(rhs.value_)); \
    return *this;                                                            \
  }

  __DEFINE_FLAGS_BITWISE_OP(|)
  __DEFINE_FLAGS_BITWISE_OP(&)
  __DEFINE_FLAGS_BITWISE_OP (^)
  //__DEFINE_FLAGS_BITWISE_OP(~)
#undef __DEFINE_FLAGS_BITWISE_OP

  FUN_ALWAYS_INLINE bool HasAny(T mask) const {
    return (static_cast<UnderlyingType>(value_) &
            static_cast<UnderlyingType>(mask)) != 0;
  }

  FUN_ALWAYS_INLINE bool HasAll(T mask) const {
    return (static_cast<UnderlyingType>(value_) &
            static_cast<UnderlyingType>(mask)) ==
           static_cast<UnderlyingType>(mask);
  }

  FUN_ALWAYS_INLINE Flags<T>& Set(T flags) {
    *this |= flags;
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& SetBool(T flags, bool set) {
    if (set) {
      value_ = static_cast<T>(static_cast<UnderlyingType>(value_) |
                              (static_cast<UnderlyingType>(flags)));
    } else {
      value_ = static_cast<T>(static_cast<UnderlyingType>(value_) &
                              (~static_cast<UnderlyingType>(flags)));
    }
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& ClearAll() {
    value_ = (T)0;
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& SetAll() {
    value_ = (T) ~((UnderlyingType)0);
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& SetAll(T flags) {
    value_ = flags;
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& Clear(T flags) {
    value_ = static_cast<T>(static_cast<UnderlyingType>(value_) &
                            (~static_cast<UnderlyingType>(flags)));
    return *this;
  }

  FUN_ALWAYS_INLINE Flags<T>& Toggle(T flags) {
    value_ = static_cast<T>(static_cast<UnderlyingType>(value_) ^
                            static_cast<UnderlyingType>(flags));
    return *this;
  }

  FUN_ALWAYS_INLINE operator T() const { return value_; }

  FUN_ALWAYS_INLINE UnderlyingType UnderlyingValue() const {
    return (UnderlyingType)value_;
  }

  FUN_ALWAYS_INLINE explicit operator bool() const {
    return (UnderlyingType)value_ != (UnderlyingType)0;
  }

  FUN_ALWAYS_INLINE bool operator!() const {
    return (UnderlyingType)value_ == (UnderlyingType)0;
  }

 private:
  T value_;
};

// TODO 아래 함수는 TextStream, Archive 타입을 알아야하므로 문제가 됨.
// cc쪽에 위치시키면 될듯한데...

// TODO

template <typename T>
std::ostream& operator<<(std::ostream& stream, Flags<T> flags) {
  typedef typename Flags<T>::UnderlyingType UnderlyingType;
  auto val = static_cast<UnderlyingType>(static_cast<T>(flags));
  const int BIT_COUNT = sizeof(UnderlyingType) * CHAR_BIT;
  char str[BIT_COUNT + 1];
  for (int i = 0; i < BIT_COUNT; ++i) {
    str[BIT_COUNT - i - 1] = ((val >> i) & 0x1) ? '1' : '0';
  }
  str[BIT_COUNT] = 0;
  stream << str;
  return stream;
}

/*
template <typename T>
TextStream& operator << (TextStream& stream, Flags<T> flags) {
  typedef typename Flags<T>::UnderlyingType UnderlyingType;
  auto val = static_cast<UnderlyingType>(static_cast<T>(flags));
  const int BIT_COUNT = sizeof(UnderlyingType) * 8;
  char str[BIT_COUNT + 1];
  for (int i = 0; i < BIT_COUNT; ++i) {
    str[BIT_COUNT-i-1] = ((val >> i) & 0x1) ? '1' : '0';
  }
  str[BIT_COUNT] = 0;
  stream << str;
  return stream;
}

template <typename T>
Archive& operator << (Archive& stream, const Flags<T> flags) {
  typedef typename Flags<T>::UnderlyingType UnderlyingType;
  stream << (UnderlyingType)flags;
  return stream;
}

template <typename T>
Archive& operator >> (Archive& stream, Flags<T>& flags) {
  typedef typename Flags<T>::UnderlyingType UnderlyingType;
  UnderlyingType val;
  stream >> val;
  flags = val;
  return stream;
}
*/

}  // namespace fun

#define __DEFINE_FLAGS_BITWISE_OP(T, Op)                            \
  FUN_ALWAYS_INLINE T operator Op(T lhs, T rhs) {                   \
    using UnderlyingType = ::fun::Flags<T>::UnderlyingType;         \
    return static_cast<T>(static_cast<UnderlyingType>(lhs)          \
                              Op static_cast<UnderlyingType>(rhs)); \
  }

#define __DEFINE_FLAGS_BITWISE_OP_IN_CLASS(T, Op)                   \
  FUN_ALWAYS_INLINE friend T operator Op(T lhs, T rhs) {            \
    using UnderlyingType = ::fun::Flags<T>::UnderlyingType;         \
    return static_cast<T>(static_cast<UnderlyingType>(lhs)          \
                              Op static_cast<UnderlyingType>(rhs)); \
  }

/**
 * 클래스 밖에서 정의할 경우에 사용함.
 */
#define FUN_DECLARE_FLAGS(FlagsType, T) \
  __DEFINE_FLAGS_BITWISE_OP(T, &)       \
  __DEFINE_FLAGS_BITWISE_OP(T, |)       \
  typedef ::fun::Flags<T> FlagsType;

/**
 * 클래스 안에서 정의할 경우에 사용함.
 */
#define FUN_DECLARE_FLAGS_IN_CLASS(FlagsType, T) \
  __DEFINE_FLAGS_BITWISE_OP_IN_CLASS(T, &)       \
  __DEFINE_FLAGS_BITWISE_OP_IN_CLASS(T, |)       \
  typedef ::fun::Flags<T> FlagsType;
