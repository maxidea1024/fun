//TODO type_traits.h로 옮기자.

#pragma once

namespace fun {
namespace tl {

/**
 * Tests if a FromType* is convertible to a ToType*
 */
template <typename FromType, typename ToType>
struct PointerIsConvertibleFromTo
{
 private:
  static uint8 Test(...);
  static uint16 Test(ToType*);

 public:
  enum { Value = sizeof(Test((FromType*)nullptr)) - 1 };
};

} // namespace tl
} // namespace fun
