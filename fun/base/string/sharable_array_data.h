#pragma once

#include "fun/base/base.h"
#include "fun/base/flags.h"
#include "fun/base/ref_counter.h"

namespace fun {

struct FUN_BASE_API UntypedSharableArrayData {
  /** Reference counter */
  RefCounter ref;

  /** Used length */
  int32 length;

  // alignment를 맞추기 위해서, 32bit 하나로 맞춰줌.

  /** Allocated length */
  uint32 alloc : 31;

  /** Whether is reserved or not */
  uint32 capacity_reserved : 1;

  /** in bytes from beginning of header */
  intptr_t offset;

  FUN_ALWAYS_INLINE void* MutableData() {
    fun_check(length == 0 || offset < 0 ||
              offset >= sizeof(UntypedSharableArrayData));
    return reinterpret_cast<char*>(this) + offset;
  }

  FUN_ALWAYS_INLINE const void* ConstData() const {
    fun_check(length == 0 || offset < 0 ||
              offset >= sizeof(UntypedSharableArrayData));
    return reinterpret_cast<const char*>(this) + offset;
  }

  // TODO 내부에서 헤더 길이를 계산할때 alignment를 고려한다면, 아래 조건을
  // 달리해야한다.
  FUN_ALWAYS_INLINE bool IsRawData() const {
    return offset != sizeof(UntypedSharableArrayData);
  }

  /**
   * This refers to array data mutability, not "header data" represented by
   * data members in UntypedSharableArrayData.
   * Shared data (array and header) must still follow COW principles.
   */
  FUN_ALWAYS_INLINE bool IsMutable() const { return alloc != 0; }

  enum AllocationOption {
    CapacityReserved = 0x01,
    Unsharable = 0x02,
    RawData = 0x04,
    Grow = 0x08,
    Default = 0x00,
  };
  // TODO 이거 뭔가 불완전하다... Flags<T> 코드를 보완해야할듯!
  // FUN_DECLARE_FLAGS_IN_CLASS(AllocationOptions, AllocationOption);
  typedef uint32 AllocationOptions;

  FUN_ALWAYS_INLINE size_t DetachCapacity(size_t new_capacity) {
    // capacity가 확보된 상태에서, 요청한 capacity가
    // 기존에 할당된 것보다 작을 경우 그대로 반환
    if (capacity_reserved && new_capacity < alloc) {
      return alloc;
    }
    return new_capacity;
  }

  FUN_ALWAYS_INLINE AllocationOptions DetachFlags() const {
    AllocationOptions result;
    if (capacity_reserved) {
      result |= CapacityReserved;
    }
    return result;
  }

  FUN_ALWAYS_INLINE AllocationOptions CloneFlags() const {
    AllocationOptions result;
    if (capacity_reserved) {
      result |= CapacityReserved;
    }
    return result;
  }

  static UntypedSharableArrayData* Allocate(
      size_t object_size, size_t alignment, size_t capacity,
      AllocationOptions options = Default) noexcept;

  static UntypedSharableArrayData* ReallocateUnaligned(
      UntypedSharableArrayData* old_data, size_t object_size, size_t alignment,
      size_t capacity, AllocationOptions options = Default) noexcept;

  static void Free(UntypedSharableArrayData* data, size_t object_size,
                   size_t alignment) noexcept;
};

template <typename T>
struct TypedSharableArrayData : public UntypedSharableArrayData {
  typedef UntypedSharableArrayData Super;

  FUN_ALWAYS_INLINE T* MutableData() {
    return static_cast<T*>(Super::MutableData());
  }

  FUN_ALWAYS_INLINE const T* ConstData() const {
    return static_cast<const T*>(Super::ConstData());
  }

  struct AlignmentHelper {
    UntypedSharableArrayData header;
    T data;
  };

  FUN_ALWAYS_INLINE static TypedSharableArrayData* Allocate(
      size_t capacity, AllocationOptions options = Default) {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    return static_cast<TypedSharableArrayData*>(Super::Allocate(
        sizeof(T), alignof(AlignmentHelper), capacity, options));
  }

  FUN_ALWAYS_INLINE static TypedSharableArrayData* ReallocateUnaligned(
      TypedSharableArrayData* old_data, size_t capacity,
      AllocationOptions options = Default) {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    return static_cast<TypedSharableArrayData*>(Super::ReallocateUnaligned(
        old_data, sizeof(T), alignof(AlignmentHelper), capacity, options));
  }

  FUN_ALWAYS_INLINE static void Free(TypedSharableArrayData* data) {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    Super::Free(data, sizeof(T), alignof(AlignmentHelper));
  }

  FUN_ALWAYS_INLINE static TypedSharableArrayData* FromRawData(
      const T* data, size_t len, AllocationOptions options = Default) {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    TypedSharableArrayData* result = Allocate(0, options | RawData);
    if (result) {
      result->offset = reinterpret_cast<const char*>(data) -
                       reinterpret_cast<const char*>(result);
      result->length = int32(len);
    }
    return result;
  }

  FUN_ALWAYS_INLINE static TypedSharableArrayData* SharedEmpty() {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    return Allocate(0);
  }

  FUN_ALWAYS_INLINE static TypedSharableArrayData* UnsharedEmpty() {
    static_assert(sizeof(TypedSharableArrayData) == sizeof(Super),
                  "sizeof(TypedSharableArrayData) == sizeof(Super)");
    return Allocate(0, Unsharable);
  }

  //
  // STL Compatibilities
  //

  typedef T* iterator;
  typedef const T* const_iterator;

  FUN_ALWAYS_INLINE iterator begin(iterator = iterator()) {
    return MutableData();
  }
  FUN_ALWAYS_INLINE iterator end(iterator = iterator()) {
    return MutableData() + length;
  }
  FUN_ALWAYS_INLINE const_iterator
  begin(const_iterator = const_iterator()) const {
    return MutableData();
  }
  FUN_ALWAYS_INLINE const_iterator
  end(const_iterator = const_iterator()) const {
    return MutableData() + length;
  }
  FUN_ALWAYS_INLINE const_iterator
  cbegin(const_iterator = const_iterator()) const {
    return ConstData();
  }
  FUN_ALWAYS_INLINE const_iterator
  cend(const_iterator = const_iterator()) const {
    return ConstData() + length;
  }
};

template <typename T, size_t N>
struct TypedStaticSharableArrayData {
  UntypedSharableArrayData header;
  T data[N];
};

// Support for returning TypedSharableArrayDataPointer<T> from functions
template <typename T>
struct TypedSharableArrayDataPointerRef {
  TypedSharableArrayData<T>* ptr;
};

#define STATIC_SHARABLE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(size,   \
                                                                  offset) \
  \ { {RefCounter::PERSISTENT_COUNTER_VALUE}, size, 0, 0, offset }

#define STATIC_SHARABLE_ARRAY_DATA_HEADER_INITIALIZER(type, size)       \
  STATIC_SHARABLE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(            \
      Size, ((sizeof(UntypedSharableArrayData) + (alignof(type) - 1)) & \
             ~(alignof(type) - 1)))

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section). This is
// accomplished by hiding a static const instance of
// TypedStaticSharableArrayData, which is POD.

#define SHARABLE_ARRAY_LITERAL(type, ...)                   \
  ([]() -> TypedSharableArrayDataPointerRef<type> {         \
    struct StaticWrapper {                                  \
      static TypedSharableArrayDataPointerRef<type> Get() { \
        SHARABLE_ARRAY_LITERAL_IMPL(type, __VA_ARGS__);     \
        return ref;                                         \
      }                                                     \
    };                                                      \
    return StaticWrapper::Get();                            \
  }())

#define SHARABLE_ARRAY_LITERAL_IMPL(type, ...)                      \
  union {                                                           \
    Type TYPE_MUST_BE_POD;                                          \
  } dummy;                                                          \
  (void)dummy;                                                      \
  type data[] = {__VA_ARGS__};                                      \
  (void)data;                                                       \
  static const TypedStaticSharableArrayData<type, size> literal = { \
      STATIC_SHARABLE_ARRAY_DATA_HEADER_INITIALIZER(type, size),    \
      {__VA_ARGS__}};                                               \
  TypedSharableArrayDataPointerRef<type> ref = {                    \
      static_cast<TypedSharableArrayData<type>*>(                   \
          const_cast<UntypedSharableArrayData*>(&literal.header))};

}  // namespace fun
