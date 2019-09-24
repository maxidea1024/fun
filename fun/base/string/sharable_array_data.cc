#include "fun/base/string/sharable_array_data.h"
#include "fun/base/memory.h"
#include <limits>

namespace fun {

namespace {

struct GrowingBlockSize {
  size_t size;
  size_t element_count;
};

size_t CalculateBlockSize(
    size_t element_count,
    size_t element_size,
    size_t header_size = 0) {
  uint32 count = uint32(element_count);
  uint32 size = uint32(element_size);
  uint32 header = uint32(header_size);
  fun_check(element_size);
  fun_check(size == element_size);
  fun_check(header == header_size);
  if (count != element_count) {
    return std::numeric_limits<size_t>::max();
  }

  uint32 bytes = 0;
  //TODO check overflow
  //if (MulOverflow(size, count, &bytes) || AddOverflow(bytes, header, &bytes)) {
  //  return std::numeric_limits<size_t>::max();
  //}
  bytes = size * count + header;

  if (int32(bytes) < 0) { // >= 2GB
    return std::numeric_limits<size_t>::max();
  }

  return bytes;
}

FUN_ALWAYS_INLINE uint32 NextPowerOfTwo(uint32 v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  ++v;
  return v;
}

FUN_ALWAYS_INLINE uint64 NextPowerOfTwo(uint64 v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  ++v;
  return v;
}

// util 함수로 빼주어도 좋을듯...
GrowingBlockSize CalculateGrowingBlockSize(
    size_t element_count,
    size_t element_size,
    size_t header_size) {
  GrowingBlockSize result = {
    std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()
  };

  uint32 bytes = uint32(CalculateBlockSize(element_count, element_size, header_size));
  if (int32(bytes) < 0) {
    return result;
  }

  //버그가 있었음!
  //const uint32 more_bytes = MathBase::RoundUpToPowerOfTwo(bytes); //이게 맞는건지?

  const uint32 more_bytes = NextPowerOfTwo(bytes); //이게 맞는건지?
  if (int32(more_bytes) < 0) {
    bytes += (more_bytes - bytes) / 2;
  } else {
    bytes = more_bytes;
  }

  result.element_count = (bytes - uint32(header_size)) / uint32(element_size);
  result.size = bytes;
  return result;
}

//const UntypedSharableArrayData UntypedSharableArrayData::g_shared_null[2] = {
//  { { RefCounter::PERSISTENT_COUNTER_VALUE }, 0, 0, 0, sizeof(UntypedSharableArrayData) }, // shared null
//  /* zero initialized terminator */
//};

static const UntypedSharableArrayData g_empty_values[3] = {
  { { RefCounter::PERSISTENT_COUNTER_VALUE }, 0, 0, 0, sizeof(UntypedSharableArrayData) }, // shared empty
  { { RefCounter::UNSHARABLE_COUNTER_VALUE }, 0, 0, 0, sizeof(UntypedSharableArrayData) }, // unshared empty
  /* zero initialized terminator */
};

static const UntypedSharableArrayData& g_empty = g_empty_values[0];
static const UntypedSharableArrayData& g_unsharable_empty = g_empty_values[1];

FUN_ALWAYS_INLINE size_t CalculateBlockSize_helper(
    size_t& capacity,
    size_t object_size,
    size_t header_size,
    uint32 options) {
  if (options & UntypedSharableArrayData::Grow) {
    auto result = CalculateGrowingBlockSize(capacity, object_size, header_size);
    capacity = result.element_count;
    return result.size;
  } else {
    return CalculateBlockSize(capacity, object_size, header_size);
  }
}

UntypedSharableArrayData* Reallocate_helper(
    UntypedSharableArrayData* header,
    size_t alloc_size,
    uint32 options) {
  header = static_cast<UntypedSharableArrayData*>(UnsafeMemory::Realloc(header, alloc_size));
  if (header) {
    header->capacity_reserved = !!(options & UntypedSharableArrayData::CapacityReserved);
  }
  return header;
}

} // namespace

UntypedSharableArrayData* UntypedSharableArrayData::Allocate(
    size_t object_size,
    size_t alignment,
    size_t capacity,
    AllocationOptions options) noexcept {
  fun_check(alignment >= alignof(UntypedSharableArrayData) && (alignment & (alignment - 1)) == 0);

  // don't allocate empty headers.
  if (!(options & RawData) && capacity == 0) {
    if (options & Unsharable) {
      return const_cast<UntypedSharableArrayData*>(&g_unsharable_empty);
    }
    return const_cast<UntypedSharableArrayData*>(&g_empty);
  }

  size_t header_size = sizeof(UntypedSharableArrayData);

  //TODO alignment는 유지해주는게 좋을듯...
  if (!(options & RawData)) {
    size_t adj = (alignment - alignof(UntypedSharableArrayData));
    fun_check(adj == 0);
    header_size += adj;
  }

  if (int32(header_size) < 0) { // 2GB 정의를 하나 두는게 좋을듯...
    return nullptr;
  }

  const size_t alloc_size = CalculateBlockSize_helper(capacity, object_size, header_size, options);

  //critical point:
  //  alignment가 생략되어서 엄청난 버그가 지속적으로 있었음.
  // alignment는 별도로 계산되므로, 여기서 alignment가 요구되지 않음.
  UntypedSharableArrayData* header = static_cast<UntypedSharableArrayData*>(UnsafeMemory::Malloc(alloc_size));
  if (header) {
    uintptr_t data = uintptr_t(header) + header_size;
    header->ref.InitAsOwned(); //ref=1
    header->length = 0;
    header->alloc = capacity;
    header->capacity_reserved = !!(options & CapacityReserved);
    header->offset = data - uintptr_t(header);
    fun_check(header->offset == header_size);
  }
  return header;
}

UntypedSharableArrayData* UntypedSharableArrayData::ReallocateUnaligned(
    UntypedSharableArrayData* old_data,
    size_t object_size,
    size_t alignment,
    size_t capacity,
    AllocationOptions options) noexcept {
  fun_check(old_data);
  fun_check(old_data->IsMutable());
  fun_check(old_data->ref.IsShared() == false);

  const size_t header_size = sizeof(UntypedSharableArrayData);
  const size_t alloc_size = CalculateBlockSize_helper(capacity, object_size, header_size, options);
  UntypedSharableArrayData* header = static_cast<UntypedSharableArrayData*>(Reallocate_helper(old_data, alloc_size, options));
  if (header) {
    header->alloc = capacity;
  }
  return header;
}

void UntypedSharableArrayData::Free(
    UntypedSharableArrayData* data,
    size_t object_size,
    size_t alignment) noexcept {
  fun_check(alignment >= alignof(UntypedSharableArrayData) && (alignment & (alignment - 1)) == 0);
  if (data == &g_unsharable_empty) {
    return;
  }

  // Persistent일 경우에는 호출되면 안됨.
  // Persistent일 경우에는 ref-counting 메커니즘 자체가
  // 무시되므로, 이곳 까지 도달할 수 없음!
  fun_check(data == nullptr || !data->ref.IsPersistent());
  UnsafeMemory::Free(data);
}

} // namespace fun
