#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"

namespace fun {

/**
 * Compile time type id.
 *
 * cross module 이슈가 있으므로, 같은 module 내에서만 제한적으로 사용해야함.
 */
class TypeId {
 public:
  template <typename T>
  static TypeId Of() {
    static char const inst{};
    return TypeId(&inst);
  }

  operator size_t() const { return reinterpret_cast<size_t>(id_); }

 private:
  using Id = void const*;

  TypeId(Id id) : id_(id) {}

  Id id_;
};

//
// inlines
//

FUN_ALWAYS_INLINE bool operator==(const TypeId& lhs, const TypeId& rhs) {
  return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);
}

FUN_ALWAYS_INLINE bool operator!=(const TypeId& lhs, const TypeId& rhs) {
  return static_cast<size_t>(lhs) != static_cast<size_t>(rhs);
}

FUN_ALWAYS_INLINE bool operator<(const TypeId& lhs, const TypeId& rhs) {
  return static_cast<size_t>(lhs) < static_cast<size_t>(rhs);
}

FUN_ALWAYS_INLINE uint32 HashOf(const TypeId& v) { return HashOf((size_t)v); }

}  // namespace fun
