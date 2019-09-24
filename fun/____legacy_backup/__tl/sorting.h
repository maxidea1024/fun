#pragma once

#include "fun/base/ftl/less.h"
#include "fun/base/ftl/greater.h"

namespace fun {
namespace tl {

/**
 * Helper class for dereferencing pointer types in Sort function
 */
template <typename T, typename Predicate>
class DereferenceWrapper
{
 public:
  DereferenceWrapper(const Predicate& pred)
    : pred_(pred)
  {
  }

  /**
   * Pass through for non-pointer types
   */
  inline bool operator()(T& a, T& b) { return pred_(a, b); }
  inline bool operator()(const T& a, const T& b) const { return pred_(a, b); }

 private:
  const Predicate& pred_;
};

/**
 * Partially specialized version of the above class
 */
template <typename T, typename Predicate>
class DereferenceWrapper<T*, Predicate>
{
 public:
  DereferenceWrapper(const Predicate& pred)
    : pred_(pred)
  {
  }

  /**
   * Dereference pointers
   */
  inline bool operator()(T* a, T* b) const
  {
    //note: 단순히 포인터 비교시에는 문제가 될 수 있음.  그러나, 좀더 고민이 필요해보임!!
    return pred_(*a, *b);
    //아니네, 포인터 타입을 Predicate에 넘길수가 없구나... 어쩐다... 흠...
    //return pred(a, b);
  }

 private:
  const Predicate& pred_;
};

/**
 * Sort elements using user defined predicate class.
 * The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 * This is the internal sorting function used by Sort overrides.
 *
 * \param first - pointer to the first element to sort.
 * \param count - the number of items to sort.
 * \param pred - predicate class.
 */
template <typename T, typename Predicate>
void SortInternal(T* first, const int32 count, const Predicate& pred)
{
  struct Stack {
    T* min;
    T* max;
  };

  if (count < 2) {
    return;
  }

  Stack recursion_stack[32] = { { first, first + count - 1 } }, current, inner;
  for (Stack* stack_top = recursion_stack; stack_top >= recursion_stack; --stack_top) {
    current = *stack_top;

  Loop:
    intptr_t count2 = current.max - current.min + 1;
    if (count2 <= 8) {
      // Use simple bubble-sort.
      while (current.max > current.min) {
        T* max;
        T* item;
        for (max = current.min, item = current.min+1; item <= current.max; item++) {
          if (pred(*max, *item)) {
            max = item;
          }
        }
        Swap(*max, *current.max--);
      }
    }
    else {
      // Grab middle element so sort doesn't exhibit worst-cast behavior with presorted lists.
      Swap(current.min[count2/2], current.min[0]);

      // Divide list into two halves, one with items <=current.min, the other with items > current.max.
      inner.min = current.min;
      inner.max = current.max + 1;
      for (;;) {
        while (++inner.min <= current.max && !pred(*current.min, *inner.min));
        while (--inner.max >  current.min && !pred(*inner.max, *current.min));
        if (inner.min > inner.max) {
          break;
        }
        Swap(*inner.min, *inner.max);
      }
      Swap(*current.min, *inner.max);

      // Save big half and recurse with small half.
      if (inner.max - 1 - current.min >= current.max - inner.min) {
        if (current.min + 1 < inner.max) {
          stack_top->min = current.min;
          stack_top->max = inner.max - 1;
          stack_top++;
        }

        if (current.max > inner.min) {
          current.min = inner.min;
          goto Loop;
        }
      }
      else {
        if (current.max > inner.min) {
          stack_top->min = inner.min;
          stack_top->max = current.max;
          stack_top++;
        }

        if (current.min + 1 < inner.max) {
          current.max = inner.max - 1;
          goto Loop;
        }
      }
    }
  }
}

/**
 * Sort elements using user defined predicate class.
 * The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 *
 * \param first - pointer to the first element to sort.
 * \param count - the number of items to sort.
 * \param pred - predicate class.
*/
template <typename T, typename Predicate>
void Sort(T* first, const int32 count, const Predicate& pred)
{
  SortInternal(first, count, DereferenceWrapper<T, Predicate>(pred));
}

/**
 * Specialized version of the above Sort function for pointers to elements.
 *
 * \param first - pointer to the first element to sort.
 * \param count - the number of items to sort.
 * \param pred - predicate class.
 */
template <typename T, typename Predicate>
void Sort(T** first, const int32 count, const Predicate& pred)
{
  SortInternal(first, count, DereferenceWrapper<T*, Predicate>(pred));
}

/**
 * Sort elements. The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 * Assumes < operator is defined for the template type.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 */
template <typename T>
void Sort(T* first, const int32 count)
{
  SortInternal(first, count, DereferenceWrapper<T, Less<T>>(Less<T>()));
}

/**
 * Specialized version of the above Sort function for pointers to elements.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 */
template <typename T>
void Sort(T** first, const int32 count)
{
  SortInternal(first, count, DereferenceWrapper<T*, Less<T>>(Less<T>()));
}

/**
 * Stable merge to perform sort below. Stable sort is slower than non-stable
 * algorithm.
 *
 * \param out - Pointer to the first element of output array.
 * \param in - Pointer to the first element to sort.
 * \param mid - Middle point of the table, i.e. merge separator.
 * \param count - Number of elements in the whole table.
 * \param pred - pred class.
 */
template <typename T, typename Predicate>
void Merge(T* out, T* in, const int32 mid, const int32 count, const Predicate& pred)
{
  int32 merged = 0;
  int32 picked;
  int32 a = 0, b = mid;

  while (merged < count) {
    if (merged != b && (b >= count || !pred(in[b], in[a]))) {
      picked = a++;
    }
    else {
      picked = b++;
    }

    out[merged] = in[picked];

    ++merged;
  }
}

/**
 * Euclidean algorithm using modulo policy.
 */
class EuclidDivisionGCD
{
 public:
  /**
   * Calculate GCD.
   *
   * \param a - first parameter.
   * \param b - second parameter.
   *
   * \returns Greatest common divisor of a and b.
   */
  static int32 GCD(int32 a, int32 b)
  {
    while (b != 0) {
      int32 tmp = b;
      b = a % b;
      a = tmp;
    }

    return a;
  }
};

/**
 * Array rotation using juggling technique.
 *
 * @template_param GCDPolicy - Policy for calculating greatest common divisor.
 */
template <typename GCDPolicy>
class JugglingRotation
{
 public:
  /**
   * Rotates array.
   *
   * \param first - Pointer to the array.
   * \param from - Rotation starting point.
   * \param to - Rotation ending point.
   * \param amount - amount of steps to rotate.
   */
  template <typename T>
  static void Rotate(T* first, const int32 from, const int32 to, const int32 amount)
  {
    if (amount == 0) {
      return;
    }

    auto count = to - from;
    auto gcd = GCDPolicy::GCD(count, amount);
    auto cycle_size = count / gcd;

    for (int32 i = 0; i < gcd; ++i) {
      T buffer_object = MoveTemp(first[from + i]);
      int32 index_to_fill = i;

      for (int32 cycle_index = 0; cycle_index < cycle_size; ++cycle_index) {
        index_to_fill = (index_to_fill + amount) % count;
        Swap(first[from + index_to_fill], buffer_object);
      }
    }
  }
};

/**
 * Merge policy for merge sort.
 *
 * @template_param TRotationPolicy - Policy for array rotation algorithm.
 */
template <typename TRotationPolicy>
class RotationInPlaceMerge
{
 public:
  /**
   * Two sorted arrays merging function.
   *
   * \param first - Pointer to array.
   * \param mid - Middle point i.e. separation point of two arrays to merge.
   * \param count - Number of elements in array.
   * \param pred - pred for comparison.
   */
  template <typename T, typename Predicate>
  static void Merge(T* first, const int32 mid, const int32 count, const Predicate& pred)
  {
    int32 a_start = 0;
    int32 b_start = mid;

    while (a_start < b_start && b_start < count) {
      const int32 new_a_offset = BinarySearchLast(first + a_start, b_start - a_start, first[b_start], pred) + 1;
      a_start += new_a_offset;

      if (a_start >= b_start) { // done
        break;
      }

      const int32 new_b_offset = BinarySearchFirst(first + b_start, count - b_start, first[a_start], pred);
      TRotationPolicy::Rotate(first, a_start, b_start + new_b_offset, new_b_offset);
      b_start += new_b_offset;
      a_start += new_b_offset + 1;
    }
  }

 private:
  /**
   * Performs binary search, resulting in position of the first element with given value in an array.
   *
   * \param first - Pointer to array.
   * \param count - Number of elements in array.
   * \param value - value to look for.
   * \param pred - pred for comparison.
   *
   * \returns Position of the first element with value value.
   */
  template <typename T, typename Predicate>
  static int32 BinarySearchFirst(T* first, const int32 count, const T& value, const Predicate& pred)
  {
    int32 start = 0;
    int32 end = count;

    while (end - start > 1) {
      const int32 mid = (start + end) / 2;
      bool flag = pred(first[mid], value);

      start = flag ? mid : start;
      end = flag ? end : mid;
    }

    return pred(first[start], value) ? start + 1 : start;
  }

  /**
   * Performs binary search, resulting in position of the last element with given value in an array.
   *
   * \param first - Pointer to array.
   * \param count - Number of elements in array.
   * \param value - value to look for.
   * \param pred - pred for comparison.
   *
   * \returns Position of the last element with value value.
   */
  template <typename T, typename Predicate>
  static int32 BinarySearchLast(T* first, const int32 count, const T& value, const Predicate& pred)
  {
    int32 start = 0;
    int32 end = count;

    while (end - start > 1) {
      const int32 mid = (start + end) / 2;
      const bool flag = !pred(value, first[mid]);

      start = flag ? mid : start;
      end = flag ? end : mid;
    }

    return pred(value, first[start]) ? start - 1 : start;
  }
};

/**
 * Merge sort class.
 *
 * @template_param MergePolicy - Merging policy.
 * @template_param MinMergeSubgroupSize - Minimal size of the subgroup that should be merged.
 */
template <typename MergePolicy, int32 MinMergeSubgroupSize = 2>
class MergeSort
{
 public:
  /**
   * Sort the array.
   *
   * \param first - Pointer to the array.
   * \param count - Number of elements in the array.
   * \param pred - pred for comparison.
   */
  template <typename T, typename Predicate>
  static void Sort(T* first, const int32 count, const Predicate& pred)
  {
    int32 sub_group_start = 0;

    if (MinMergeSubgroupSize > 1) {
      if (MinMergeSubgroupSize > 2) {
        // first pass with simple bubble-sort.
        do {
          int32 group_end = MathBase::Min(sub_group_start + MinMergeSubgroupSize, count);
          do {
            for (int32 it = sub_group_start; it < group_end - 1; ++it) {
              if (pred(first[it + 1], first[it])) {
                Swap(first[it], first[it + 1]);
              }
            }
            group_end--;
          } while (group_end - sub_group_start > 1);

          sub_group_start += MinMergeSubgroupSize;
        } while (sub_group_start < count);
      }
      else {
        for (int32 sub_group = 0; sub_group < count; sub_group += 2) {
          if (sub_group + 1 < count && pred(first[sub_group + 1], first[sub_group])) {
            Swap(first[sub_group], first[sub_group + 1]);
          }
        }
      }
    }

    int32 sub_group_size = MinMergeSubgroupSize;
    while (sub_group_size < count) {
      sub_group_start = 0;
      do {
        MergePolicy::Merge(
          first + sub_group_start,
          sub_group_size,
          MathBase::Min(sub_group_size << 1, count - sub_group_start),
          pred);
        sub_group_start += sub_group_size << 1;
      } while (sub_group_start < count);

      sub_group_size <<= 1;
    }
  }
};

/**
 * Stable sort elements using user defined predicate class. The sort is stable,
 * meaning that the ordering of equal items is preserved, but it's slower than
 * non-stable algorithm.
 *
 * This is the internal sorting function used by StableSort overrides.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 * \param pred - predicate class
 */
template <typename T, typename Predicate>
void StableSortInternal(T* first, const int32 count, const Predicate& pred)
{
  MergeSort<RotationInPlaceMerge<JugglingRotation<EuclidDivisionGCD>>>::Sort(first, count, pred);
}

/**
 * Stable sort elements using user defined predicate class. The sort is stable,
 * meaning that the ordering of equal items is preserved, but it's slower than
 * non-stable algorithm.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 * \param pred - predicate class
 */
template <typename T, typename Predicate>
void StableSort(T* first, const int32 count, const Predicate& pred)
{
  StableSortInternal(first, count, DereferenceWrapper<T, Predicate>(pred));
}

/**
 * Specialized version of the above StableSort function for pointers to elements.
 * Stable sort is slower than non-stable algorithm.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 * \param pred - predicate class
 */
template <typename T, typename Predicate>
void StableSort(T** first, const int32 count, const Predicate& pred)
{
  StableSortInternal(first, count, DereferenceWrapper<T*, Predicate>(pred));
}

/**
 * Stable sort elements. The sort is stable, meaning that the ordering of equal
 * items is preserved, but it's slower than non-stable algorithm.
 *
 * Assumes < operator is defined for the template type.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 */
template <typename T>
void StableSort(T* first, const int32 count)
{
  StableSortInternal(first, count, DereferenceWrapper<T, Less<T>>(Less<T>()));
}

/**
 * Specialized version of the above StableSort function for pointers to elements.
 * Stable sort is slower than non-stable algorithm.
 *
 * \param first - pointer to the first element to sort
 * \param count - the number of items to sort
 */
template <typename T>
void StableSort(T** first, const int32 count)
{
  StableSortInternal(first, count, DereferenceWrapper<T*, Less<T>>(Less<T>()));
}

} // namespace tl
} // namespace fun
