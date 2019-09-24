//TODO type_traits.h로 옮겨주자.

#pragma once

#include "fun/base/ftl/integral_constant.h"

namespace fun {
namespace tl {

template <typename T> struct IsEnum { enum { value = __is_enum(T) }; };

//TODO 적용을 해야하나 말아야하나?
template <typename WrappedEnumType>
struct IsWrappedEnum : FalseType
{
};

} // namespace tl
} // namespace fun
