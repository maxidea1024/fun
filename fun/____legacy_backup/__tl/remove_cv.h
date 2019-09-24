//TODO type_traits.h로 옮기자.

#pragma once

namespace fun {
namespace tl {

/**
 * RemoveCV<type> will remove any const/volatile qualifiers from a type.
 * (based on std::remove_cv<>
 * note: won't remove the const from "const int*", as the pointer is not const
 */
template <typename T> struct RemoveCV                   { typedef T Type; };
template <typename T> struct RemoveCV<const T         > { typedef T Type; };
template <typename T> struct RemoveCV<volatile T      > { typedef T Type; };
template <typename T> struct RemoveCV<const volatile T> { typedef T Type; };

} // namespace tl
} // namespace fun
