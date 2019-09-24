#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"  // Invoke

namespace fun {
namespace algo {

/**
 * Conditionally copies a range into a container.
 *
 * \param input - Any iterable type.
 * \param output - Container to hold the output.
 * \param pred - Condition which returns true for elements that should be copied
 * and false for elements that should be skipped.
 */
template <typename Input, typename Output, typename Predicate>
FUN_ALWAYS_INLINE void CopyIf(const Input& input, Output& output,
                              Predicate pred) {
  for (auto&& elem : input) {
    if (Invoke(pred, elem)) {
      output.Add(elem);
    }
  }
}

/**
 * Copies a range into a container.
 *
 * \param input - Any iterable type.
 * \param output - Container to hold the output.
 */
template <typename Input, typename Output>
FUN_ALWAYS_INLINE void Copy(const Input& input, Output& output) {
  for (auto&& elem : input) {
    output.Add(elem);
  }
}

}  // namespace algo
}  // namespace fun
