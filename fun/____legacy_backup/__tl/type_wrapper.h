#pragma once

namespace fun {
namespace tl {

/**
 * Wraps a type.
 * 
 * The intended use of this template is to allow types to be passed around where the unwrapped type would give different behavior.
 * An example of this is when you want a template specialization to refer to the primary template, but doing that would cause
 * infinite recursion through the specialization, e.g.:
 * 
 *   // Before
 *   template <typename T>
 *   struct Thing
 *   {
 *     void f(T t)
 *     {
 *        DoSomething(t);
 *     }
 *   };
 * 
 *   template <>
 *   struct Thing<int>
 *   {
 *     void f(int t)
 *     {
 *       DoSomethingElseFirst(t);
 *       Thing<int>::f(t); // Infinite recursion
 *     }
 *   };
 * 
 * 
 *   // After
 *   template <typename T>
 *   struct Thing
 *   {
 *     typedef typename UnwrapType<T>::Type RealType;
 * 
 *     void f(RealType t)
 *     {
 *        DoSomething(t);
 *     }
 *   };
 * 
 *   template <>
 *   struct Thing<int>
 *   {
 *     void f(int t)
 *     {
 *       DoSomethingElseFirst(t);
 *       Thing<TypeWrapper<int>>::f(t); // works
 *     }
 *   };
 */
template <typename T>
struct TypeWrapper;

template <typename T>
struct UnwrapType
{
  typedef T Type;
};

template <typename T>
struct UnwrapType<TypeWrapper<T>>
{
  typedef T Type;
};

} // namespace tl
} // namespace fun
