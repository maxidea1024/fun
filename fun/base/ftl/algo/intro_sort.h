#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/algo/impl/binary_heap.h"

namespace fun {
namespace algo {

namespace impl {

template <typename T, typename Projection, typename Predicate>
void IntroSortInternal(T* first, size_t count, Projection proj, Predicate pred) {
  struct Stack {
    T* min;
    T* max;
    uint32 max_depth;
  };

  if (count < 2) {
    return;
  }

  //TODO Loge의 double 버젼을 만들어주는게 좋을듯함.
  //Stack recursion_stack[32] = {{first, first + count - 1, (uint32)(MathBase::Loge((float)count) * 2.f)}}, current, inner;
  Stack recursion_stack[32] = {{first, first + count - 1, (uint32)(MathBase::Loge((float)count) * 2.f)}}, current, inner;
  for (Stack* stack_top = recursion_stack; stack_top >= recursion_stack; --stack_top) {
    current = *stack_top;

  Loop:
    intptr_t count = current.max - current.min + 1;

    if (current.max_depth == 0) {
      // We're too deep into quick sort, switch to heap sort
      HeapSortInternal(current.min, count, proj, pred);
      continue;
    }

    if (count <= 8) {
      // Use simple bubble-sort.
      while (current.max > current.min) {
        T *max, *item;
        for (max = current.min, item = current.min + 1; item <= current.max; ++item) {
          if (pred(Invoke(proj, *max), Invoke(proj, *item))) {
            max = item;
          }
        }
        Swap(*max, *current.max--);
      }
    } else {
      // Grab middle element so sort doesn't exhibit worst-case behavior with presorted lists.
      Swap(current.min[count/2], current.min[0]);

      // Divide list into two halves, one with items <=current.min, the other with items >current.max.
      inner.min = current.min;
      inner.max = current.max + 1;
      for (;;) {
        while (++inner.min<=current.max && !pred(Invoke(proj, *current.min), Invoke(proj, *inner.min)));
        while (--inner.max> current.min && !pred(Invoke(proj, *inner.max), Invoke(proj, *current.min)));
        if (inner.min>inner.max) {
          break;
        }
        Swap(*inner.min, *inner.max);
      }
      Swap(*current.min, *inner.max);

      --current.max_depth;

      // Save big half and recurse with small half.
      if (inner.max - 1 - current.min >= current.max - inner.min) {
        if (current.min + 1 < inner.max) {
          stack_top->min = current.min;
          stack_top->max = inner.max - 1;
          stack_top->max_depth = current.max_depth;
          ++stack_top;
        }

        if (current.max > inner.min) {
          current.min = inner.min;
          goto Loop;
        }
      } else {
        if (current.max > inner.min) {
          stack_top->min = inner  .min;
          stack_top->max = current.max;
          stack_top->max_depth = current.max_depth;
          ++stack_top;
        }

        if (current.min + 1 < inner.max) {
          current.max = inner.max - 1;
          goto Loop;
        }
      }
    }
  }
}

} // namespace impl

template <typename Range>
FUN_ALWAYS_INLINE void IntroSort(Range& range) {
  impl::IntroSortInternal(GetMutableData(range), GetCount(range), IdentityFunctor(), Less<>());
}

template <typename Range, typename Predicate>
FUN_ALWAYS_INLINE void IntroSort(Range& range, Predicate pred) {
  impl::IntroSortInternal(GetMutableData(range), GetCount(range), IdentityFunctor(), MoveTemp(pred));
}

template <typename Range, typename Projection>
FUN_ALWAYS_INLINE void IntroSortBy(Range& range, Projection proj) {
  impl::IntroSortInternal(GetMutableData(range), GetCount(range), MoveTemp(proj), Less<>());
}

template <typename Range, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void IntroSortBy(Range& range, Projection proj, Predicate pred) {
  impl::IntroSortInternal(GetMutableData(range), GetCount(range), MoveTemp(proj), MoveTemp(pred));
}

} // namespace algo
} // namespace fun
