//TODO type_traits.h로 옮겨주자.

#pragma once

#include "fun/base/ftl/and_or_not.h"
#include "IsArithmetic.h"
#include "IsPointer.h"

#include <type_traits>  

#if _MSC_VER == 1900
// __is_pod changed in VS2015, however the results are still correct for all usages I've been able to locate.
#pragma warning(push)
#pragma warning(disable:4647)
#endif // _MSC_VER == 1900

namespace fun {
namespace tl {

/**
 * Traits class which tests if a type is POD.
 */
template <typename T>
struct IsPOD
{
  enum { Value = std::is_pod<T>::Value };
};

} // namespace tl
} // namespace fun

#if _MSC_VER == 1900
#pragma warning(pop)
#endif // _MSC_VER == 1900
