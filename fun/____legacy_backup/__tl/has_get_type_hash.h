#include "CoreTypes.h"
#include "Templates/AndOrNot.h"
#include "Templates/IsArithmetic.h"
#include "Templates/IsPointer.h"
#include "Templates/IsEnum.h"

namespace fun {
namespace tl {

namespace GetTypeHashExists_internal {

  struct NotSpecified {};

  template <typename T>
  struct ReturnValueCheck
  {
    static char (&Func())[2];
  };

  template <>
  struct ReturnValueCheck<NotSpecified>
  {
    static char (&Func())[1];
  };

  template <typename T>
  NotSpecified HashOf(const T&);

  template <typename T>
  const T& Make();

  template <typename T, bool IsHashableScalarType = Or<IsArithmetic<T>, IsPointer<T>, IsEnum<T>>::Value>
  struct GetTypeHashQuery
  {
    // All arithmetic, pointer and enums types are hashable
    enum { Value = true };
  };

  template <typename T>
  struct GetTypeHashQuery<T, false>
  {
    enum { Value = sizeof(ReturnValueCheck<decltype(HashOf(Make<T>()))>::Func()) == sizeof(char[2]) };
  };

} // namespace GetTypeHashExists_internal

/**
 * Traits class which tests if a type has a HashOf overload.
 */
template <typename T>
struct HasGetTypeHash
{
  enum { Value = GetTypeHashExists_internal::GetTypeHashQuery<T>::Value };
};

} // namespace tl
} // namespace fun
