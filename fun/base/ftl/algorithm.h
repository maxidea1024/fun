#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Works just like std::min_element.
 */
template <typename ForwardIt>
FUN_ALWAYS_INLINE ForwardIt MinElement(ForwardIt first, ForwardIt end) {
  ForwardIt result = first;
  for (; ++first != end;) {
    if (*first < *result) {
      result = first;
    }
  }
  return result;
}

/**
 * Works just like std::min_element.
 */
template <typename ForwardIt, typename Predicate>
FUN_ALWAYS_INLINE ForwardIt MinElement(ForwardIt first, ForwardIt end,
                                       const Predicate& pred) {
  ForwardIt result = first;
  for (; ++first != end;) {
    if (pred(*first, *result)) {
      result = first;
    }
  }
  return result;
}

/**
 * Works just like std::max_element.
 */
template <typename ForwardIt>
FUN_ALWAYS_INLINE ForwardIt MaxElement(ForwardIt first, ForwardIt end) {
  ForwardIt result = first;
  for (; ++first != end;) {
    if (*result < *first) {
      result = first;
    }
  }
  return result;
}

/**
 * Works just like std::max_element.
 */
template <typename ForwardIt, typename Predicate>
FUN_ALWAYS_INLINE ForwardIt MaxElement(ForwardIt first, ForwardIt end,
                                       const Predicate& pred) {
  ForwardIt result = first;
  for (; ++first != end;) {
    if (pred(*result, *first)) {
      result = first;
    }
  }
  return result;
}

}  // namespace fun
