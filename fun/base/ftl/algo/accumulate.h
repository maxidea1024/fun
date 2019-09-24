#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"

namespace fun {
namespace algo {

/**
 * Sums a range by successively applying op.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 * \param op - Summing Operation (the default is Plus<>)
 *
 * \return the result of summing all the elements of input
 */
template <typename T, typename A, typename Op>
FUN_ALWAYS_INLINE T Accumulate(const A& input, T init, Op op) {
  T result = MoveTemp(init);
  for (auto&& elem : input) {
    result = op(MoveTemp(result), elem);
  }

  return result;
}

/**
 * Sums a range.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 *
 * \return the result of summing all the elements of input
 */
template <typename T, typename A>
FUN_ALWAYS_INLINE T Accumulate(const A& input, T init) {
  return Accumulate(input, MoveTemp(init), Plus<>());
}

/**
 * Sums a range by applying map_op to each element, and then summing the
 * results.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 * \param map_op - Mapping Operation
 * \param op - Summing Operation (the default is Plus<>)
 *
 * \return the result of mapping and then summing all the elements of input
 */
template <typename T, typename A, typename MapOp, typename Op>
FUN_ALWAYS_INLINE T TransformAccumulate(const A& input, MapOp map_op, T init,
                                        Op op) {
  T result = MoveTemp(init);
  for (auto&& element : input) {
    result = op(MoveTemp(result), map_op(element));
  }

  return result;
}

/**
 * Sums a range by applying map_op to each element, and then summing the
 * results.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 * \param map_op - Mapping Operation
 *
 * \return the result of mapping and then summing all the elements of input
 */
template <typename T, typename A, typename MapOp>
FUN_ALWAYS_INLINE T TransformAccumulate(const A& input, MapOp map_op, T init) {
  return TransformAccumulate(input, map_op, MoveTemp(init), Plus<>());
}

}  // namespace algo
}  // namespace fun
