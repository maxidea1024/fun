#pragma once

#include "core_types.h"
#include "pointer_is_convertible_from_to.h"
#include "template.h"
#include "decay.h"

namespace fun {
namespace tl {

namespace Invoke_internal {

  template <typename BaseT, typename CallableT>
  inline auto DereferenceIfNecessary(CallableT&& callable)
    -> typename EnableIf<PointerIsConvertibleFromTo<typename Decay<CallableT>::Type, typename Decay<BaseT>::Type>::Value, decltype((CallableT&&)callable)>::Type
  {
    return (CallableT&&)callable;
  }

  template <typename BaseT, typename CallableT>
  inline auto DereferenceIfNecessary(CallableT&& callable)
    -> typename EnableIf<!PointerIsConvertibleFromTo<typename Decay<CallableT>::Type, typename Decay<BaseT>::Type>::Value, decltype(*(CallableT&&)callable)>::Type
  {
    return *(CallableT&&)callable;
  }

} // namespace Invoke_internal

/**
 * Invokes a callable with a set of arguments.  Allows the following:
 *
 * - Calling a functor object given a set of arguments.
 * - Calling a function pointer given a set of arguments.
 * - Calling a member function given a reference to an object and a set of arguments.
 * - Calling a member function given a pointer (including smart pointers) to an object and a set of arguments.
 * - Projecting via a data member pointer given a reference to an object.
 * - Projecting via a data member pointer given a pointer (including smart pointers) to an object.
 *
 * See: http://en.cppreference.com/w/cpp/utility/functional/invoke
 */
template <typename FunctionT, typename... Args>
inline decltype(auto) Invoke(FunctionT&& func, Args&&... args)
{
  return Forward<FunctionT>(func)(Forward<Args>(args)...);
}

template <typename ReturnT, typename ObjectT, typename CallableT>
inline decltype(auto) Invoke(ReturnT ObjectT::*pdm, CallableT&& callable)
{
  return Invoke_internal::DereferenceIfNecessary<ObjectT>(Forward<CallableT>(callable)).*pdm;
}

template <typename ReturnT, typename ObjectT, typename... PMFArgs, typename CallableT, typename... Args>
inline decltype(auto) Invoke(ReturnT (ObjectT::*method)(PMFArgs...), CallableT&& callable, Args&&... args)
{
  return (Invoke_internal::DereferenceIfNecessary<ObjectT>(Forward<CallableT>(callable)).*method)(Forward<Args>(args)...);
}

template <typename ReturnT, typename ObjectT, typename... PMFArgs, typename CallableT, typename... Args>
inline decltype(auto) Invoke(ReturnT (ObjectT::*method)(PMFArgs...) const, CallableT&& callable, Args&&... args)
{
  return (Invoke_internal::DereferenceIfNecessary<ObjectT>(Forward<CallableT>(callable)).*method)(Forward<Args>(args)...);
}

} // namespace tl
} // namespace fun
