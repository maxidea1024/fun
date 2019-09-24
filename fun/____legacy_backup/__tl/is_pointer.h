//TODO type_traits.h로 옮겨주자.

#pragma once

namespace fun {
namespace tl {

//TODO namespace를 무효화하고, IsPoint -> IsPointerType 으로 변경하는게 좋을듯..

/**
 * Traits class which tests if a type is a pointer.
 */
template <typename T> struct IsPointer : FalseType {};

template <typename T> struct IsPointer< T*              > : TrueType {};
template <typename T> struct IsPointer<const T*         > : TrueType {};
template <typename T> struct IsPointer<volatile T*      > : TrueType {};
template <typename T> struct IsPointer<const volatile T*> : TrueType {};

template <typename T> struct IsPointer<const T         > { enum { Value = IsPointer<T>::Value }; };
template <typename T> struct IsPointer<volatile T      > { enum { Value = IsPointer<T>::Value }; };
template <typename T> struct IsPointer<const volatile T> { enum { Value = IsPointer<T>::Value }; };

} // namespace tl
} // namespace fun
