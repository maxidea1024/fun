//TODO type_traits로 옮겨야하나? C++ std::enable_if 는 "type_traits" 파일에 위치함.

#pragma once

namespace fun {
namespace tl {

template <bool pred, typename ResultT = void> struct EnableIf;
template <typename ResultT> struct EnableIf<true,  ResultT> { typedef ResultT Type; };
template <typename ResultT> struct EnableIf<false, ResultT> {};

template <bool pred, typename Func> struct LazyEnableIf;
template <typename Func> struct LazyEnableIf<true,  Func> { typedef typename Func::Type Type; };
template <typename Func> struct LazyEnableIf<false, Func> {};

} // namespace tl
} // namespace fun
