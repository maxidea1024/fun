#pragma once

namespace fun {
namespace tl {

template <typename T>
struct IsAbstract
{
  enum { Value = __is_abstract(T) };
};

} // namespace tl
} // namespace fun
