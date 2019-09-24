// TODO 아직 완성된 버젼이 아님...

#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

template <typename ArrayAllocator>
template <typename T, typename Container = Array<T, ArrayAllocator>,
          typename Comparator = Less<T>>
class PriorityQueue {
 public:
  PriorityQueue() = default;

  PriorityQueue(const PriorityQueue&) = delete;
  PriorityQueue& operator=(const PriorityQueue&) = delete;

  PriorityQueue(PriorityQueue&&) = default;
  PriorityQueue& operator=(PriorityQueue&&) = default;

  void Insert(const T& item) {
    int32 n = items_.Push(what);
    HeapifyUp(n);
  }

  T Pop() {
    T ret = MoveTemp(items_[0]);
    items_.RemoveAtSwap(0);
    HeapifyDown(0);
    return ret;
  }

  const T& Top() const { return items_[0]; }

  template <typename T2>
  bool Remove(const T2& item) {
    for (int32 i = 0; i < items_.Count(); ++i) {
      if (
    }
  }

  void Clear() { items_.Clear(); }

 private:
  Container items_;

  FUN_ALWAYS_INLINE static int32 Parent(int32 n) { return (n - 1) / 2; }
  FUN_ALWAYS_INLINE static int32 Left(int32 n) { return 2 * n + 1; }
  FUN_ALWAYS_INLINE static int32 Right(int32 n) { return 2 * n + 2; }

  FUN_ALWAYS_INLINE void SwapItems(int32 a, int32 b) {
    T tmp(items_[a]);
    items_[a] = b;
    items_[b] = tmp;
  }

  FUN_ALWAYS_INLINE bool Larger(int32 a, int32 b) {
    return Comparator::Compare(items_[a], items_[b]) > 0;
  }

  void HeapifyUp(int32 n) {
    int32 current = 0;
    while (current > 0) {
      int32 parent = Parent(current);
      int32 larger = current;
      if (((current ^ 1) < items_.Count()) && Larger(current ^ 1, larger)) {
        larger = current ^ 1;
      }

      if (Larger(larger, parent)) {
        SwapItems(larger, parent);
      } else {
        return;
      }

      current = parent;
    }
  }

  void HeapifyDown(int32 n) {
    int32 current = n;
    do {
      int32 l = Left(current);
      int32 r = Right(current);
      int32 larger = current;
      if ((l < items_.Count()) && Larger(l, larger)) {
        larger = l;
      }

      if ((r < items_.Count()) && Larger(r, larger)) {
        larger = r;
      }

      if (larger == current) {
        return;
      }

      SwapItems(larger, current);
      current = larger;
    } while (current < items.GetSize());
  }
};

}  // namespace fun
