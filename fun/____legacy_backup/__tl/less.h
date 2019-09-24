//TODO template.h로 옮기자.

#pragma once

namespace fun {
namespace tl {

/**
 * Default predicate class. Assumes < operator is defined for the template type.
 */
template <typename T>
struct Less
{
  inline bool operator()(const T& a, const T& b) const
  {
    return a < b;
  }
};

} // namespace tl
} // namespace fun
