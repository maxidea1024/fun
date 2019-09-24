#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"  // Swap

namespace fun {
namespace algo {

/**
 * Rearranges the elements so that all the elements for which predicate
 * returns true precede all those for which it returns false.  (not stable)
 *
 * \param elements - pointer to the first element
 * \param count - the number of items
 * \param pred - unary predicate class
 *
 * \return Index of the first element in the second group
 */
template <typename T, typename UnaryPredicate>
int32 Partition(T* elements, const int32 count, const UnaryPredicate& pred) {
  T* first = elements;
  T* end = elements + count;

  while (first != end) {
    while (pred(*first)) {
      ++first;
      if (first == end) {
        return first - elements;
      }
    }

    do {
      --end;
      if (first == end) {
        return first - elements;
      }
    } while (!pred(*end));

    fun::Swap(*first, *end);
    ++first;
  }

  return first - elements;
}

}  // namespace algo
}  // namespace fun
