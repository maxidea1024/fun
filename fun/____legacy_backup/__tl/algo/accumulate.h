#pragma once

namespace fun {

// Plus<T> specifically takes const T& and returns T.
// Plus<> (empty angle brackets) is late-binding, taking whatever is passed and returning the correct result type for (A+B)
template <typename T = void>
struct Plus
{
  inline T operator()(const T& x, const T& y) { return x + y; }
};

template <>
struct Plus<void>
{
  template <typename U, typename V>
  inline auto operator()(U&& x, V&& y) -> decltype(x + y) { return x + y; }
};

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
inline T Accumulate(const A& input, T init, Op op)
{
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
inline T Accumulate(const A& input, T init)
{
  return Accumulate(input, MoveTemp(init), Plus<>());
}

/**
 * Sums a range by applying map_op to each element, and then summing the results.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 * \param map_op - Mapping Operation
 * \param op - Summing Operation (the default is Plus<>)
 *
 * \return the result of mapping and then summing all the elements of input
 */
template <typename T, typename A, typename MapT, typename Op>
inline T TransformAccumulate(const A& input, MapT map_op, T init, Op op)
{
  T result = MoveTemp(init);
  for (auto&& elem : input) {
    result = op(MoveTemp(result), map_op(elem));
  }

  return result;
}

/**
 * Sums a range by applying map_op to each element, and then summing the results.
 *
 * \param input - Any iterable type
 * \param init - Initial value for the summation
 * \param map_op - Mapping Operation
 *
 * \return the result of mapping and then summing all the elements of input
 */
template <typename T, typename A, typename MapOp>
inline T TransformAccumulate(const A& input, MapOp map_op, T init)
{
  return TransformAccumulate(input, map_op, MoveTemp(init), Plus<>());
}

} // namespace algo
} // namespace fun
