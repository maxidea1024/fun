#pragma once

namespace fun {
namespace algo {

namespace Find_internal {

  template <typename RangeType, typename Predicate>
  const typename RangeType::ElementType* FindIf(const RangeType& range, const Predicate& pred)
  {
    for (const auto& elem : range) {
      if (pred(elem)) {
        return &elem;
      }
    }

    return nullptr;
  }

} // namespace Find_internal


/**
 * Returns a pointer to the first element matching a value in a range.
 *
 * \param range - The range to search.
 * \param value - The value to search for.
 *
 * \return A pointer to the first element found, or nullptr if none was found.
 */
template <typename RangeType, typename ValueType>
inline typename RangeType::ElementType* Find(RangeType& range, const ValueType& value)
{
  return const_cast<typename RangeType::ElementType*>(Find(const_cast<const RangeType&>(range), value));
}

/**
 * Returns a pointer to the first element matching a value in a range.
 *
 * \param range - The range to search.
 * \param value - The value to search for.
 *
 * \return A pointer to the first element found, or nullptr if none was found.
 */
template <typename RangeType, typename ValueType>
inline const typename RangeType::ElementType* Find(const RangeType& range, const ValueType& value)
{
  return Find_internal::FindIf(range, [&](const typename RangeType::ElementType& elem){
    return elem == value;
  });
}

/**
 * Returns a pointer to the first element matching a predicate in a range.
 *
 * \param range - The range to search.
 * \param pred - The predicate to search for.
 *
 * \return A pointer to the first element found, or nullptr if none was found.
 */
template <typename RangeType, typename Predicate>
inline typename RangeType::ElementType* FindIf(RangeType& range, const Predicate& pred)
{
  return const_cast<typename RangeType::ElementType*>(FindIf(const_cast<const RangeType&>(range), pred));
}

/**
 * Returns a pointer to the first element matching a predicate in a range.
 *
 * \param range - The range to search.
 * \param pred - The predicate to search for.
 *
 * \return A pointer to the first element found, or nullptr if none was found.
 */
template <typename RangeType, typename Predicate>
inline const typename RangeType::ElementType* FindIf(const RangeType& range, const Predicate& pred)
{
  return Find_internal::FindIf(range, pred);
}

} // namespace algo
} // namespace fun
