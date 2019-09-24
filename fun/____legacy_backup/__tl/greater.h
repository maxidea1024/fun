//TODO std::greater는 "functional.h" 파일에 위치함.

#pragma once

namespace fun {
namespace tl {

/**
 * Utility predicate class. Assumes < operator is defined for the template type.
 */
template <typename T>
struct Greater
{
  inline bool operator()(const T& a, const T& b) const
  {
    return b < a;
  }
};

} // namespace tl
} // namespace fun
