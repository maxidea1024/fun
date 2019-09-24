//TODO type_traits.h로 옮기자.

#pragma once

namespace fun {
namespace tl {

template <typename T> struct MakeUnsigned { typedef T Type; };

template <> struct MakeUnsigned<int8 > { typedef uint8  Type; };
template <> struct MakeUnsigned<int16> { typedef uint16 Type; };
template <> struct MakeUnsigned<int32> { typedef uint32 Type; };
template <> struct MakeUnsigned<int64> { typedef uint64 Type; };

} // namespace tl
} // namespace fun
