#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h" // DeclVal

namespace fun {
namespace algo {

namespace impl {

template <typename Range>
struct RangePointerType {
  using Type = decltype(&*begin(DeclVal<Range&>()));
};

template <typename T, size_t N>
struct RangePointerType<T[N]> {
  using Type = T*;
};

} // namespace impl

} // namespace algo
} // namespace fun
