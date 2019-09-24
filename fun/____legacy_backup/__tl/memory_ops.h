#pragma once

#include <new>

#include "HAL/FunMemory.h"
#include "HAL/PlatformTypes.h"
#include "Templates/Template.h"
#include "TypeTraits.h"
#include "AndOrNot.h"
#include "IsTriviallyCopyAssignable.h"
#include "IsTriviallyCopyConstructible.h"
#include "IsTriviallyDestructible.h"

namespace fun {

namespace MemoryOps_internal {

  template <typename DestinationElementType, typename SourceElementType>
  struct CanBitwiseRelocate
  {
    enum {
      Value =
        Or<
          IsSame<DestinationElementType, SourceElementType>,
          And<
            IsBitwiseConstructible<DestinationElementType, SourceElementType>,
            IsTriviallyDestructible<SourceElementType>
          >
        >::Value
    };
  };

} // MemoryOps_internal

/**
 * Default constructs a range of items in memory.
 *
 * \param element - The address of the first memory location to construct at.
 * \param count - The number of elements to destruct.
 */
template <typename ElementType>
inline typename EnableIf<!IsZeroConstructible<ElementType>::Value>::Type
  DefaultConstructItems(void* elements, int32 count)
{
  ElementType* element = (ElementType*)elements;
  while (count > 0) {
    new (element) ElementType;
    ++element;
    --count;
  }
}

template <typename ElementType>
inline typename EnableIf<IsZeroConstructible<ElementType>::Value>::Type
  DefaultConstructItems(void* elements, int32 count)
{
  UnsafeMemory::Memzero(elements, sizeof(ElementType) * count);
}

/**
 * Destructs a single item in memory.
 *
 * \param element - a pointer to the item to destruct.
*/
template <typename ElementType>
inline typename EnableIf<!IsTriviallyDestructible<ElementType>::Value>::Type
  DestructItem(ElementType* element)
{
  // We need a typedef here because VC won't compile the destructor
  // call below if ElementType itself has a member called ElementType
  typedef ElementType DestructItemsElementTypeTypedef;

  element->DestructItemsElementTypeTypedef::~DestructItemsElementTypeTypedef();
}

template <typename ElementType>
inline typename EnableIf<IsTriviallyDestructible<ElementType>::Value>::Type
  DestructItem(ElementType* element)
{
  // do not call destructor.
}


/**
 * Destructs a range of items in memory.
 *
 * \param elements - a pointer to the first item to destruct.
 * \param count - The number of elements to destruct.
 */
template <typename ElementType>
inline typename EnableIf<!IsTriviallyDestructible<ElementType>::Value>::Type
  DestructItems(ElementType* elements, int32 count)
{
  while (count > 0) {
    // We need a typedef here because VC won't compile the destructor
    // call below if ElementType itself has a member called ElementType
    typedef ElementType DestructItemsElementTypeTypedef;

    elements->DestructItemsElementTypeTypedef::~DestructItemsElementTypeTypedef();
    ++elements;
    --count;
  }
}

template <typename ElementType>
inline typename EnableIf<IsTriviallyDestructible<ElementType>::Value>::Type
  DestructItems(ElementType* elements, int32 count)
{
  // do not call destructor.
}

/**
 * Constructs a range of items into memory from a set of arguments.  The arguments come from an another array.
 *
 * \param dst - The memory location to start copying into.
 * \param src - a pointer to the first argument to pass to the constructor.
 * \param count - The number of elements to copy.
 */
template <typename DestinationElementType, typename SourceElementType>
inline typename EnableIf<!IsBitwiseConstructible<DestinationElementType, SourceElementType>::Value>::Type
  ConstructItems(void* dst, const SourceElementType* src, int32 count)
{
  while (count > 0) {
    new (dst) DestinationElementType(*src);
    ++(DestinationElementType*&)dst;
    ++src;
    --count;
  }
}

template <typename DestinationElementType, typename SourceElementType>
inline typename EnableIf<IsBitwiseConstructible<DestinationElementType, SourceElementType>::Value>::Type
  ConstructItems(void* dst, const SourceElementType* src, int32 count)
{
  UnsafeMemory::Memcpy(dst, src, sizeof(SourceElementType) * count);
}

/**
 * Copy assigns a range of items.
 *
 * \param dst - The memory location to start assigning to.
 * \param src - a pointer to the first item to assign.
 * \param count - The number of elements to assign.
 */
template <typename ElementType>
inline typename EnableIf<!IsTriviallyCopyAssignable<ElementType>::Value>::Type
  CopyAssignItems(ElementType* dst, const ElementType* src, int32 count)
{
  while (count > 0) {
    *dst = *src;
    ++src;
    ++dst;
    --count;
  }
}

template <typename ElementType>
inline typename EnableIf<IsTriviallyCopyAssignable<ElementType>::Value>::Type
  CopyAssignItems(ElementType* dst, const ElementType* src, int32 count)
{
  UnsafeMemory::Memcpy(dst, src, sizeof(ElementType) * count);
}

/**
 * Relocates a range of items to a new memory location as a new type. This is a so-called 'destructive move' for which
 * there is no single operation in C++ but which can be implemented very efficiently in general.
 *
 * \param dst - The memory location to relocate to.
 * \param src - a pointer to the first item to relocate.
 * \param count - The number of elements to relocate.
 */
template <typename DestinationElementType, typename SourceElementType>
inline typename EnableIf<!MemoryOps_internal::CanBitwiseRelocate<DestinationElementType, SourceElementType>::Value>::Type
  RelocateConstructItems(void* dst, const SourceElementType* src, int32 count)
{
  while (count > 0) {
    // We need a typedef here because VC won't compile the destructor
    // call below if SourceElementType itself has a member called SourceElementType
    typedef SourceElementType RelocateConstructItemsElementTypeTypedef;

    new (dst) DestinationElementType(*src);
    ++(DestinationElementType*&)dst;
    (src++)->RelocateConstructItemsElementTypeTypedef::~RelocateConstructItemsElementTypeTypedef();
    --count;
  }
}

template <typename DestinationElementType, typename SourceElementType>
inline typename EnableIf<MemoryOps_internal::CanBitwiseRelocate<DestinationElementType, SourceElementType>::Value>::Type
  RelocateConstructItems(void* dst, const SourceElementType* src, int32 count)
{
  // All existing FUN containers seem to assume trivial relocatability (i.e. memcpy'able) of their members,
  // so we're going to assume that this is safe here.  However, it's not generally possible to assume this
  // in general as objects which contain pointers/references to themselves are not safe to be trivially
  // relocated.
  //
  // However, it is not yet possible to automatically infer this at compile time, so we can't enable
  // different (i.e. safer) implementations anyway.

  UnsafeMemory::Memmove(dst, src, sizeof(SourceElementType) * count);
}

/**
 * Move constructs a range of items into memory.
 *
 * \param dst - The memory location to start moving into.
 * \param src - a pointer to the first item to move from.
 * \param count - The number of elements to move.
 */
template <typename ElementType>
inline typename EnableIf<!IsTriviallyCopyConstructible<ElementType>::Value>::Type
  MoveConstructItems(void* dst, const ElementType* src, int32 count)
{
  while (count > 0) {
    new (dst) ElementType((ElementType&&)*src);
    ++(ElementType*&)dst;
    ++src;
    --count;
  }
}

template <typename ElementType>
inline typename EnableIf<IsTriviallyCopyConstructible<ElementType>::Value>::Type
  MoveConstructItems(void* dst, const ElementType* src, int32 count)
{
  UnsafeMemory::Memmove(dst, src, sizeof(ElementType) * count);
}

/**
 * Move assigns a range of items.
 *
 * \param dst - The memory location to start move assigning to.
 * \param src - a pointer to the first item to move assign.
 * \param count - The number of elements to move assign.
 */
template <typename ElementType>
inline typename EnableIf<!IsTriviallyCopyAssignable<ElementType>::Value>::Type
  MoveAssignItems(ElementType* dst, const ElementType* src, int32 count)
{
  while (count > 0) {
    *dst = (ElementType&&)*src;
    ++src;
    ++dst;
    --count;
  }
}

template <typename ElementType>
inline typename EnableIf<IsTriviallyCopyAssignable<ElementType>::Value>::Type
  MoveAssignItems(ElementType* dst, const ElementType* src, int32 count)
{
  UnsafeMemory::Memmove(dst, src, sizeof(ElementType) * count);
}

template <typename ElementType>
inline typename EnableIf<TypeTraits<ElementType>::IsBytewiseComparable, bool>::Type
  CompareItems(const ElementType* a, const ElementType* b, int32 count)
{
  return UnsafeMemory::Memcmp(a, b, sizeof(ElementType) * count) == 0;
}

template <typename ElementType>
inline typename EnableIf<!TypeTraits<ElementType>::IsBytewiseComparable, bool>::Type
  CompareItems(const ElementType* a, const ElementType* b, int32 count)
{
  while (count > 0) {
    if (!(*a == *b)) {
      return false;
    }

    ++a;
    ++b;
    --count;
  }

  return true;
}

} // namespace fun
