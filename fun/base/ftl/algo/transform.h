#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"  // Invoke

namespace fun {
namespace algo {

template <typename Input, typename Output, typename Predicate,
          typename TransformT>
FUN_ALWAYS_INLINE void TransformIf(const Input& input, Output& output,
                                   const Predicate& pred,
                                   const TransformT& xform) {
  for (auto&& elem : input) {
    if (Invoke(pred, elem)) {
      output.Add(Invoke(xform, elem));
    }
  }
}

template <typename Input, typename Output, typename TransformT>
FUN_ALWAYS_INLINE void Transform(const Input& input, Output& output,
                                 const TransformT& xform) {
  for (auto&& elem : input) {
    output.Add(Invoke(xform, elem));
  }
}

}  // namespace algo
}  // namespace fun
