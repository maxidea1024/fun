#pragma once

namespace fun {
namespace algo {

//전재조건:
// - collection은 ranged-for를 지원해야함.
// - element는 equal연산자를 지원해야함.

template <typename Collection, typename Element>
inline size_t Count(const Collection& collection, const Element& value)
{
  size_t count = 0;
  for (const auto& element : collection) {
    if (element == value) {
      ++count;
    }
  }

  return count;
}

template <typename Collection, typename Predicate>
inline size_t CountIf(const Collection& collection, const Predicate& pred)
{
  size_t count = 0;
  for (const auto& element : collection) {
    if (pred(element)) {
      ++count;
    }
  }

  return count;
}

} // namespace algo
} // namespace fun
