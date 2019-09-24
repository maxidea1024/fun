#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
//#include "fun/base/container/allocation_policies.h"
#include "fun/base/ftl/type_compatible_storage.h"

namespace fun {

/**
 * An array with a static number of elements.
 */
template <typename _ElementType, uint32 ElementCount,
          uint32 Alignment = alignof(_ElementType)>
class StaticArray {
 public:
  using ElementType = _ElementType;

  /**
   * Default constructor.
   */
  StaticArray() {
    // Call the default constructor for
    // each element using the in-place new operator.
    for (uint32 i = 0; i < ElementCount; ++i) {
      new (&(*this)[i]) ElementType;
    }
  }

  /**
   * Move constructor.
   */
  StaticArray(StaticArray&& other) {
    MoveConstructItems((void*)elements_, (const ElementType*)other.elements_,
                       ElementCount);
  }

  /**
   * Copy constructor.
   */
  StaticArray(const StaticArray& other) {
    ConstructItems<ElementType>(
        (void*)elements_, (const ElementType*)other.elements_, ElementCount);
  }

  /**
   * Move assignment operator.
   */
  StaticArray& operator=(StaticArray&& other) {
    MoveAssignItems((ElementType*)elements_,
                    (const ElementType*)other.elements_, ElementCount);
    return *this;
  }

  /**
   * Assignment operator.
   */
  StaticArray& operator=(const StaticArray& other) {
    CopyAssignItems((ElementType*)elements_,
                    (const ElementType*)other.elements_, ElementCount);
    return *this;
  }

  /**
   * Destructor.
   */
  ~StaticArray() { DestructItems((ElementType*)elements_, ElementCount); }

  /**
   * Serializer.
   */
  friend Archive& operator&(Archive& ar, StaticArray& a) {
    for (uint32 i = 0; i < ElementCount; ++i) {
      ar& a[i];
    }

    return ar;
  }

  // Accessors.
  ElementType& operator[](uint32 index) {
    fun_check(index < ElementCount);
    return *(ElementType*)&elements_[index];
  }

  const ElementType& operator[](uint32 index) const {
    fun_check(index < ElementCount);
    return *(const ElementType*)&elements_[index];
  }

  // Comparisons.
  friend bool operator==(const StaticArray& x, const StaticArray& y) {
    for (uint32 i = 0; i < ElementCount; ++i) {
      if (!(x[i] == y[i])) {
        return false;
      }
    }

    return true;
  }

  friend bool operator!=(const StaticArray& x, const StaticArray& y) {
    for (uint32 i = 0; i < ElementCount; ++i) {
      if (!(x[i] == y[i])) {
        return true;
      }
    }

    return false;
  }

  ElementType* MutableData() { return (ElementType*)&elements_[0]; }

  const ElementType* ConstData() const {
    return (const ElementType*)&elements_[0];
  }

  /**
   * The number of elements in the array.
   */
  int32 Count() const { return ElementCount; }

  /**
   * Hash function.
   */
  friend uint32 HashOf(const StaticArray& array) {
    uint32 result = 0;
    for (uint32 i = 0; i < ElementCount; ++i) {
      result ^= HashOf(array[i]);
    }
    return result;
  }

 private:
  AlignedStorage<sizeof(ElementType), Alignment> elements_[ElementCount];
};

/**
 * A shortcut for initializing a StaticArray with 2 elements.
 */
template <typename ElementType>
class StaticArray2 : public StaticArray<ElementType, 2> {
  typedef StaticArray<ElementType, 2> BaseType;

 public:
  StaticArray2(typename CallTraits<ElementType>::ParamType v0,
               typename CallTraits<ElementType>::ParamType v1) {
    (*this)[0] = v0;
    (*this)[1] = v1;
  }

  StaticArray2(StaticArray2&&) = default;
  StaticArray2(const StaticArray2&) = default;
  StaticArray2& operator=(StaticArray2&&) = default;
  StaticArray2& operator=(const StaticArray2&) = default;
};

/**
 * A shortcut for initializing a StaticArray with 3 elements.
 */
template <typename ElementType>
class StaticArray3 : public StaticArray<ElementType, 3> {
  typedef StaticArray<ElementType, 3> BaseType;

 public:
  StaticArray3(typename CallTraits<ElementType>::ParamType v0,
               typename CallTraits<ElementType>::ParamType v1,
               typename CallTraits<ElementType>::ParamType v2) {
    (*this)[0] = v0;
    (*this)[1] = v1;
    (*this)[2] = v2;
  }

  StaticArray3(StaticArray3&&) = default;
  StaticArray3(const StaticArray3&) = default;
  StaticArray3& operator=(StaticArray3&&) = default;
  StaticArray3& operator=(const StaticArray3&) = default;
};

/**
 * A shortcut for initializing a StaticArray with 4 elements.
 */
template <typename ElementType>
class StaticArray4 : public StaticArray<ElementType, 4> {
  typedef StaticArray<ElementType, 4> BaseType;

 public:
  StaticArray4(typename CallTraits<ElementType>::ParamType v0,
               typename CallTraits<ElementType>::ParamType v1,
               typename CallTraits<ElementType>::ParamType v2,
               typename CallTraits<ElementType>::ParamType v3) {
    (*this)[0] = v0;
    (*this)[1] = v1;
    (*this)[2] = v2;
    (*this)[3] = v3;
  }

  StaticArray4(StaticArray4&&) = default;
  StaticArray4(const StaticArray4&) = default;
  StaticArray4& operator=(StaticArray4&&) = default;
  StaticArray4& operator=(const StaticArray4&) = default;
};

/**
 * Creates a static array filled with the specified value.
 */
template <typename ElementType, uint32 ElementCount>
StaticArray<ElementType, ElementCount> MakeUniformStaticArray(
    typename CallTraits<ElementType>::ParamType value) {
  StaticArray<ElementType, ElementCount> result;
  for (uint32 i = 0; i < ElementCount; ++i) {
    result[index] = value;
  }

  return result;
}

}  // namespace fun
