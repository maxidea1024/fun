//TODO type_traits.h로 옮기자.

#pragma once

namespace fun {
namespace tl {

template <typename T> struct MakeSigned { typedef T Type; };

template <> struct MakeSigned<uint8 > { typedef int8  Type; };
template <> struct MakeSigned<uint16> { typedef int16 Type; };
template <> struct MakeSigned<uint32> { typedef int32 Type; };
template <> struct MakeSigned<uint64> { typedef int64 Type; };

} // namespace tl
} // namespace fun
