//TODO type_traits.h로 옮겨주자.

#pragma once

#include "fun/base/ftl/and_or_not.h"
#include "fun/base/ftl/is_pod_type.h"

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type has a trivial copy constructor.
 */
template <typename T>
struct IsTriviallyCopyConstructible
{
  enum { Value = OrValue<__has_trivial_copy(T), IsPOD<T>>::Value };
};

} // namespace fun
