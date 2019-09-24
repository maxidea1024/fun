#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * RGBA color made up of Float16
 */
class Float16Color {
 public:
  Float16 r;
  Float16 g;
  Float16 b;
  Float16 a;

  Float16Color();
  Float16Color(const Float16Color& src);
  Float16Color(const LinearColor& src);

  Float16Color& operator = (const Float16Color& src);
  bool operator == (const Float16Color& src) const;
};


//
// inlines
//

FUN_ALWAYS_INLINE Float16Color::Float16Color() {}

FUN_ALWAYS_INLINE Float16Color::Float16Color(const Float16Color& src) {
  r = src.r;
  g = src.g;
  b = src.b;
  a = src.a;
}

FUN_ALWAYS_INLINE Float16Color::Float16Color(const LinearColor& src)
  : r(src.r), g(src.g), b(src.b), a(src.a) {}

FUN_ALWAYS_INLINE Float16Color& Float16Color::operator = (const Float16Color& src) {
  r = src.r; g = src.g; b = src.b; a = src.a;
  return *this;
}

FUN_ALWAYS_INLINE bool Float16Color::operator == (const Float16Color& src) const {
  return r == src.r && g == src.g && b == src.b && a == src.a;
}

} // namespace fun
