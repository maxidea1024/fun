#include "fun/base/math/box2.h"

namespace fun {

Box2::Box2(const Vector2* points, const int32 count)
  : min(0.f, 0.f),
    max(0.f, 0.f),
    is_valid(false) {
  for (int32 i = 0; i < count; ++i) {
    *this += points[i];
  }
}

Box2::Box2(const Array<Vector2>& points)
  : min(0.f, 0.f),
    max(0.f, 0.f),
    is_valid(false) {
  for (const auto& point : points) {
    *this += point;
  }
}

} // namespace fun
