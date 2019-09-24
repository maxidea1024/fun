#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

template <typename T>
struct DefaultDelete {
  DefaultDelete() = default;
  DefaultDelete(const DefaultDelete&) = default;
  DefaultDelete& operator=(const DefaultDelete&) = default;
  ~DefaultDelete() = default;

  template <typename U, typename = typename EnableIf<
                            PointerIsConvertibleFromTo<U, T>::Value>::Type>
  DefaultDelete(const DefaultDelete<U>&) {}

  template <typename U, typename = typename EnableIf<
                            PointerIsConvertibleFromTo<U, T>::Value>::Type>
  DefaultDelete& operator=(const DefaultDelete<U>&) {
    return *this;
  }

  void operator()(T* ptr) const { delete ptr; }
};

template <typename T>
struct DefaultDelete<T[]> {
  DefaultDelete() = default;
  DefaultDelete(const DefaultDelete&) = default;
  DefaultDelete& operator=(const DefaultDelete&) = default;
  ~DefaultDelete() = default;

  template <typename U, typename = typename EnableIf<
                            PointerIsConvertibleFromTo<U[], T[]>::Value>::Type>
  DefaultDelete(const DefaultDelete<U>&) {}

  template <typename U, typename = typename EnableIf<
                            PointerIsConvertibleFromTo<U[], T[]>::Value>::Type>
  DefaultDelete& operator=(const DefaultDelete<U>&) {
    return *this;
  }

  template <typename U>
  typename EnableIf<PointerIsConvertibleFromTo<U[], T[]>::Value>::Type
  operator()(U* ptr) const {
    delete[] ptr;
  }
};

}  // namespace fun
