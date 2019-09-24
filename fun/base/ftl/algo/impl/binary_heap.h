#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"

namespace fun {
namespace algo {

namespace impl {

FUN_ALWAYS_INLINE int32 HeapGetLeftChildIndex(int32 index) {
  return index * 2 + 1;
}

FUN_ALWAYS_INLINE bool HeapIsLeaf(int32 index, int32 count) {
  return HeapGetLeftChildIndex(index) >= count;
}

FUN_ALWAYS_INLINE int32 HeapGetParentIndex(int32 index) {
  return (index - 1) / 2;
}

template <typename RangeValue, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapSiftDown(RangeValue* heap, int32 index, const int32 count, const Projection& proj, const Predicate& pred) {
  while (!HeapIsLeaf(index, count)) {
    const int32 left_child_index = HeapGetLeftChildIndex(index);
    const int32 right_child_index = left_child_index + 1;

    int32 min_child_index = left_child_index;
    if (right_child_index < count) {
      min_child_index = pred(Invoke(proj, heap[left_child_index]), Invoke(proj, heap[right_child_index])) ? left_child_index : right_child_index;
    }

    if (!pred(Invoke(proj, heap[min_child_index]), Invoke(proj, heap[index]))) {
      break;
    }

    fun::Swap(heap[index], heap[min_child_index]);
    index = min_child_index;
  }
}

template <typename RangeValue, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE int32 HeapSiftUp(RangeValue* heap, int32 root_index, int32 node_index, const Projection& proj, const Predicate& pred) {
  while (node_index > root_index) {
    int32 parent_index = HeapGetParentIndex(node_index);
    if (!pred(Invoke(proj, heap[node_index]), Invoke(proj, heap[parent_index]))) {
      break;
    }

    fun::Swap(heap[node_index], heap[parent_index]);
    node_index = parent_index;
  }

  return node_index;
}

template <typename RangeValue, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapifyInternal(RangeValue* first, size_t count, Projection proj, Predicate pred) {
  for (int32 index = HeapGetParentIndex((int32)count - 1); index >= 0; --index) {
    HeapSiftDown(first, index, (int32)count, proj, pred);
  }
}

template <typename RangeValue, typename Projection, typename Predicate>
FUN_ALWAYS_INLINE void HeapSortInternal(RangeValue* first, size_t count, Projection proj, Predicate pred) {
  // Reverse the predicate to build a max-heap instead of a min-heap
  ReversePredicate<Predicate> reverse_pred(pred);
  HeapifyInternal(first, count, proj, reverse_pred);

  for (int32 index = (int32)count - 1; index >= 0; --index) {
    fun::Swap(first[0], first[index]);
    HeapSiftDown(first, 0, index, proj, reverse_pred);
  }
}

} // namespace impl

} // namespace algo
} // namespace fun
