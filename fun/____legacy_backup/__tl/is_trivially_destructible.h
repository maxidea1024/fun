//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type has a trivial destructor.
 */
template <typename T>
struct IsTriviallyDestructible
{
  enum { Value = __has_trivial_destructor(T) };
};

} // namespace tl
} // namespace fun
